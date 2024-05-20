#include "osal.h"

using namespace OSAL;

struct _timer_handle_s               ///< timer handle helper structure
{
    TimerHandle_t tim;               ///< FreeRTOS timer handle
    uint32_t      next_period_ms;    ///< next period of timer
    void(*cb)(osal_timer_t, void*);  ///< timer's body
    void* ctx;                       ///< user's context
};

static void _tim_adapter(TimerHandle_t tim)  ///< FreeRTOS to OSAL timer's callback adapter
{
    if(not tim)
        return;

    auto* timer = static_cast<_timer_handle_s *>(pvTimerGetTimerID(tim));
    if(not timer)
        return;

    if(timer->cb)
        timer->cb(timer, timer->ctx);
}

void Task::task_adapter(void* ctx) noexcept
{
    auto* task = static_cast<Task*>(ctx);
    task->setup();
    task->run();
    task->teardown();
    task->m_handle = nullptr;
    Task::destroy(nullptr);
}

void Task::setup() noexcept {}

void Task::teardown() noexcept {}


task_t Task::create_from_heap(const init_t* init, body_t func, void* ctx) noexcept
{
    if(not init
       or init->heap != nullptr   // only default heap supported
       or not func)            // task body not exist
        return nullptr;

    TaskHandle_t task = nullptr;
    BaseType_t ret = xTaskCreate(func, init->name ? init->name : "null", init->stack_depth, ctx, init->priority, &task);
    if(pdPASS == ret)
        return task;
    return nullptr;
}

void Task::destroy(task_t handle) noexcept
{
    vTaskDelete(static_cast<TaskHandle_t>(handle));  // NOTE: yes, handle can be NULL
}

void Task::delay_ms(uint32_t ms) noexcept
{
    vTaskDelay(_ms2ticks(ms));
}

Task::Task() noexcept : m_handle{nullptr}
{}

Task::~Task() noexcept
{
    if(m_handle)
        destroy(m_handle);
}

bool Task::start(const init_t& init) noexcept
{
    if(init.heap or not init.name)
        return false;

    m_handle = create_from_heap(&init, task_adapter, this);
    return static_cast<bool>(m_handle);
}


void Timer::timer_adapter(timer_n_t handle, void* ctx)
{
    auto* timer = static_cast<Timer*>(ctx);
    assert(handle == timer->m_handle);
    timer->run(timer->m_user_ctx);
}

timer_n_t Timer::create(const init_t* init, body_t func, void* ctx) noexcept
{
    if(not init
       or not func)
        return nullptr;

    auto* timer = static_cast<_timer_handle_s *>(malloc(sizeof(_timer_handle_s)));
    if(not timer)
        return nullptr;

    timer->cb             = func;
    timer->ctx            = ctx;
    timer->next_period_ms = init->period_ms;
    timer->tim = xTimerCreate(init->name ? init->name : "null", pdMS_TO_TICKS(timer->next_period_ms),
                              not init->is_one_shot, timer, _tim_adapter);
    if(not timer->tim)
    {
        free(timer);
        return nullptr;
    }
    return timer;
}

void Timer::destroy(timer_n_t handle) noexcept
{
    auto* timer_handle = static_cast<_timer_handle_s*>(handle);
    if(not timer_handle or not timer_handle->tim)
        return;

    BaseType_t ret = xTimerDelete(timer_handle->tim, portMAX_DELAY);
    assert(pdPASS == ret);
    free(timer_handle);
}

bool Timer::start(timer_n_t handle, uint32_t timeout_ms) noexcept
{
    auto* timer_handle = static_cast<_timer_handle_s *>(handle);
    if(not timer_handle or not timer_handle->tim)
        return false;

    if( pdFALSE != xTimerIsTimerActive(timer_handle->tim)                  // timer running
        and pdPASS  != xTimerStop(timer_handle->tim, pdMS_TO_TICKS(timeout_ms)))  // and stop failed
        return false;
    // set new period and start
    return pdPASS == xTimerChangePeriod(timer_handle->tim, pdMS_TO_TICKS(timer_handle->next_period_ms), pdMS_TO_TICKS(timeout_ms));
}

bool Timer::stop(timer_n_t handle, uint32_t timeout_ms) noexcept
{
    auto* timer_handle = static_cast<_timer_handle_s *>(handle);
    if(not timer_handle or not timer_handle->tim)
        return false;

    return pdPASS == xTimerStop(timer_handle->tim, pdMS_TO_TICKS(timeout_ms));
}

bool Timer::set_period(timer_n_t handle, uint32_t period_ms) noexcept
{
    auto* timer_handle = static_cast<_timer_handle_s *>(handle);
    if(not timer_handle or not timer_handle->tim)
        return false;

    timer_handle->next_period_ms = period_ms;
    return true;
}

Timer::Timer(const init_t& init, void *ctx) noexcept : m_handle{create(&init, timer_adapter, this)}, m_user_ctx{ctx} {}

Timer::~Timer() noexcept
{
    destroy(m_handle);
}

bool Timer::start(uint32_t timeout_ms) const noexcept
{
    return start(m_handle, timeout_ms);
}

bool Timer::stop(uint32_t timeout_ms) const noexcept
{
    return stop(m_handle, timeout_ms);
}

bool Timer::set_period(uint32_t period_ms) const noexcept
{
    return set_period(m_handle, period_ms);
}
