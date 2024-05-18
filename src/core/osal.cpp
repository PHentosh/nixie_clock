#include "osal.h"

using namespace OSAL;

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
