#ifndef EXPERIMENTS_OSAL_H
#define EXPERIMENTS_OSAL_H

#include <cstdint>
#include <cstddef>
#include <cassert>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/timers.h>


typedef void* osal_task_t;   ///< task handle type
typedef void* osal_queue_t;  ///< queue handle type
typedef void* osal_timer_t;  ///< timer handle type

/**
 * @brief task's body function
 *
 * @param [in,out] ctx user context
 */
typedef void(*osal_task_body_t)(void* ctx);

/**
 * @brief timer's body function
 *
 * @param [in]     timer timer handle
 * @param [in,out] ctx   user context
 */
typedef void(*osal_timer_body_t)(osal_timer_t timer, void* ctx);

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

/**
 * @brief initialization parameters for software timer
 */
typedef struct
{
    bool        is_one_shot;  ///< one shot / continues flag
    uint32_t    period_ms;    ///< default period in ms
    const char* name;         ///< timer's name

} osal_timer_init_t;

inline uint32_t _ms2ticks(uint32_t ms)
{
    return ms != UINT32_MAX ? pdMS_TO_TICKS(ms) : portMAX_DELAY;
}

namespace OSAL {
    using task_t = osal_task_t;
    using timer_n_t = osal_timer_t;


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


    /**
     * @class Timer
     * @brief software timer
     *
     * You need to inherit this class and overload @ref run method (timer's body).
     * Try to minimize timer's body for prevent blocking other timers.
     */
    class [[nodiscard]] Timer
    {
    private:
        timer_n_t m_handle;    ///< timer itself
        void*   m_user_ctx;  ///< user's context

        /**
         * @brief adapter from OSAL timer function to object's method
         *
         * @param [in]     handle timer handler
         * @param [in,out] ctx    timer's context
         */
        static void timer_adapter(timer_n_t handle, void* ctx);

    protected:
        virtual void run(void* ctx) const = 0;  ///< @brief will be call on timers period expired

    public:
        using init_t = osal_timer_init_t;
        using body_t = osal_timer_body_t;

        /**
         * @copydoc osal_timer_create
         */
        [[nodiscard]] static timer_n_t create(const init_t* init, body_t func, void* ctx) noexcept;
        static void destroy(timer_n_t handle) noexcept;                                   ///< @copydoc osal_timer_destroy
        [[nodiscard]] static bool start(timer_n_t handle, uint32_t timeout_ms) noexcept;  ///< @copydoc osal_timer_start
        [[nodiscard]] static bool stop(timer_n_t handle, uint32_t timeout_ms) noexcept;   ///< @copydoc osal_timer_stop
        /**
         * @copybrief osal_timer_set_period
         *
         * This call doesn't start/stop the timer: after this call you must call @ref start to apply the new period
         *
         * @param [in] timer     timer handler
         * @param [in] period_ms new period in ms
         *
         * @retval true  on success
         * @retval false timer is corrupter (nullptr etc)
         */
        [[nodiscard]] static bool set_period(timer_n_t handle, uint32_t period_ms) noexcept;

        /**
         * @brief construct timer
         *
         * @param [in] init initialization parameters
         * @param [in] ctx user's context
         */
        Timer(const init_t& init, void* ctx) noexcept;
        virtual ~Timer() noexcept;  ///< @brief destruct timer

        Timer(const Timer&)            = delete;  ///< copy forbidden
        Timer(Timer&&)                 = delete;  ///< move forbidden
        Timer& operator=(const Timer&) = delete;  ///< copy assigning forbidden
        Timer& operator=(Timer&&)      = delete;  ///< move assigning forbidden

        /**
         * @brief start timer
         *
         * @param [in] timeout_ms timeout in ms for timer to start
         *
         * @retval true  success
         * @retval false timeout expired
         */
        [[nodiscard]] bool start(uint32_t timeout_ms) const noexcept;

        /**
         * @brief stop timer
         *
         * @param [in] timeout_ms timeout in ms for timer to stop
         *
         * @retval true  success
         * @retval false timeout expired
         */
        [[nodiscard]] bool stop(uint32_t timeout_ms) const noexcept;

        /**
         * @brief set (update) timer's period
         *
         * this method doesn't start/stop the timer: you must call @ref start to apply the new period
         *
         * @param [in] period_ms  new period in ms
         *
         * @retval true  success
         * @retval false timer is corrupter (nullptr etc)
         */
        [[nodiscard]] bool set_period(uint32_t period_ms) const noexcept;
    };

}

#endif //EXPERIMENTS_OSAL_H
