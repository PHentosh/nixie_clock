#ifndef EXPERIMENTS_OSAL_H
#define EXPERIMENTS_OSAL_H

#include <cstdint>
#include <cstddef>
#include <cassert>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>


typedef void* osal_task_t;   ///< task handle type
typedef void* osal_queue_t;  ///< queue handle type

/**
 * @brief task's body function
 *
 * @param [in,out] ctx user context
 */
typedef void(*osal_task_body_t)(void* ctx);


/**
 * @brief initialization parameter for task
 */
typedef struct
{
    void*       heap;         ///< heap pointer (NULL for default heap)
    size_t      stack_depth;  ///< task's stack depth in stack-words(!)
    const char* name;         ///< task's name
    uint32_t    priority;     ///< priority (smaller number - less priority)

} osal_task_init_t;

inline uint32_t _ms2ticks(uint32_t ms)
{
    return ms != UINT32_MAX ? pdMS_TO_TICKS(ms) : portMAX_DELAY;
}

namespace OSAL {
    using task_t = osal_task_t;

    class [[nodiscard]] Task {
    private:
        /**
         * @brief adapter from OSAL task function to object's methods
         *
         * @param [in] ctx context of task (pointer to this object)
         */
        static void task_adapter(void *ctx) noexcept;

    protected:
        task_t m_handle;  ///< task itself

        virtual void setup() noexcept;      ///< @brief will be called once at the start of the task
        virtual void run() noexcept = 0;  ///< @brief will be called after @ref setup and before @ref teardown
        virtual void teardown() noexcept;      ///< @brief will be called once at the end of the task

    public:
        using init_t = osal_task_init_t;
        using body_t = osal_task_body_t;

        /**
         * @copydoc osal_task_create_from_heap
         */
        [[nodiscard]] static task_t create_from_heap(const init_t *init, body_t func, void *ctx) noexcept;

        static void destroy(task_t handle) noexcept;  ///< @copydoc osal_task_destroy
        static void delay_ms(uint32_t ms) noexcept;  ///< @copydoc osal_delay_ms

        Task() noexcept;                        ///< @brief construct task
        virtual ~Task() noexcept;               ///< @brief destruct task

        Task(const Task &) = delete;            ///< copy forbidden
        Task(Task &&) = delete;                  ///< move forbidden
        Task &operator=(const Task &) = delete;  ///< copy assigning forbidden
        Task &operator=(Task &&) = delete;       ///< move assigning forbidden

        /**
         * @brief add task to scheduler and start it
         *
         * @param [in] init status of starting
         */
        [[nodiscard]] bool start(const init_t &init) noexcept;
    };



    /**
     * @class Queue
     * @brief dynamically allocated queue
     *
     * @tparam T   item type
     * @tparam Len length of queue
     */
    template<typename T, size_t Len>
    class [[nodiscard]] Queue
    {
        osal_queue_t m_handle;  ///< queue itself

    public:
        /**
         * @copybrief osal_queue_create_from_heap
         *
         * @param [in] heap pointer to heap (nullptr - default)
         *
         * @return queue handle
         */
        [[nodiscard]] static osal_queue_t create_from_heap(void* heap) noexcept
        {
            if(heap != nullptr                   // only deault heap supported
               or not Len or not sizeof(T))      // zero-sized queue?
                return nullptr;

            return xQueueCreate(Len, sizeof(T));
        }

        static void destroy(osal_queue_t handle) noexcept  ///< @copydoc osal_queue_destroy
        {
            if(not handle)
                return;
            vQueueDelete(static_cast<QueueHandle_t>(handle));
        }

        /**
         * @copydoc osal_queue_send
         */
        [[nodiscard]] static bool send(osal_queue_t handle, const T* item_p, uint32_t timeout_ms) noexcept
        {
            if(not handle or not item_p)
                return false;

            return pdTRUE == xQueueSend(static_cast<QueueHandle_t>(handle), item_p, _ms2ticks(timeout_ms));
        }

        /**
         * @copydoc osal_queue_recv
         */
        [[nodiscard]] static bool receive(osal_queue_t handle, T* item_p, uint32_t timeout_ms) noexcept
        {
            if(not handle or not item_p)
                return false;

            return pdTRUE == xQueueReceive(static_cast<QueueHandle_t>(handle), item_p, _ms2ticks(timeout_ms));
        }

        /**
         * @brief construct queue
         *
         * @param [in] heap heap pointer
         */
        explicit Queue(void* heap) noexcept : m_handle{create_from_heap(heap)} { assert(m_handle); }
        ~Queue() noexcept { destroy(m_handle); }  ///< @brief destruct queue

        Queue(const Queue&)            = delete;  ///< copy forbidden
        Queue(Queue&&)                 = delete;  ///< move forbidden
        Queue& operator=(const Queue&) = delete;  ///< copy assigning forbidden
        Queue& operator=(Queue&&)      = delete;  ///< move assigning forbidden

        /**
         * @brief send item to queue
         *
         * @param [in] item_p pointer to item
         * @param [in] timeout_ms timeout in ms for item to be sent
         *
         * @retval true  success
         * @retval false timeout expired
         */
        [[nodiscard]] bool send(const T* item_p, uint32_t timeout_ms) const noexcept
        {
            return send(m_handle, item_p, timeout_ms);
        }

        /**
         * @brief receive item from queue
         *
         * @param [out] item_p pointer to item
         * @param [in]  timeout_ms timeout in ms for item to be received
         *
         * @retval true  success
         * @retval false timeout expired
         */
        [[nodiscard]] bool receive(T* item_p, uint32_t timeout_ms) const noexcept
        {
            return receive(m_handle, item_p, timeout_ms);
        }
    };
}

#endif //EXPERIMENTS_OSAL_H
