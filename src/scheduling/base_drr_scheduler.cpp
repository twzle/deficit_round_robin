#include "scheduling/base_drr_scheduler.hpp"

BaseDRRScheduler::BaseDRRScheduler(
    std::string standard_name, double tti,
    double channel_sync_interval,
    std::string base_modulation_scheme)
    : standard_name(standard_name), tti_duration(tti),
      channel_sync_interval(channel_sync_interval),
      base_modulation_scheme(base_modulation_scheme) {}

/*
Планирование очереди через запись в массив очередей
и вычисление новой суммы общего количества пакетов
*/
void BaseDRRScheduler::schedule(PacketQueue &&packet_queue)
{
    scheduled_queues.push_back(packet_queue);
    total_packets += packet_queue.size();
}

void BaseDRRScheduler::connect_users(int user_count)
{
    for (int i = 0; i < user_count; ++i)
    {
        User user(base_modulation_scheme);
        connected_users.emplace(user.get_id(), std::move(user));
    }
}

User *BaseDRRScheduler::get_user_ptr(int user_id)
{
    auto it = connected_users.find(user_id);
    if (it != connected_users.end())
    {
        // Получаем указатель на пользователя
        User *user_ptr = &it->second;
        return user_ptr;
    }

    return nullptr;
}

void BaseDRRScheduler::set_resource_block_per_tti_limit(int resource_blocks_per_tti_limit)
{
    this->resource_blocks_per_tti = resource_blocks_per_tti_limit;
}

void BaseDRRScheduler::set_initial_queue(size_t new_initial_queue_id)
{
    this->current_initial_absolute_queue_id = new_initial_queue_id;
}

size_t BaseDRRScheduler::get_initial_queue()
{
    return this->current_initial_absolute_queue_id;
}

size_t BaseDRRScheduler::get_relative_queue_id(size_t current_absolute_queue_id)
{
    size_t bias = this->current_initial_absolute_queue_id;
    size_t current_relative_queue_id = current_absolute_queue_id + bias;

    if (current_relative_queue_id >= scheduled_queues.size())
    {
        return current_relative_queue_id % scheduled_queues.size();
    }
    else
    {
        return current_relative_queue_id;
    }
}

void BaseDRRScheduler::check_queue_remaining_scheduled_packets(
    PacketQueue &queue, double current_time, TTIStats &tti_stats)
{
    PacketQueue tmp(queue.get_quant(), queue.get_limit());

    int packet_count = 0;

    // Проверка пакетов исходной очереди на факт доступности
    while (queue.size() > 0)
    {
        Packet packet = queue.front();

        // Если пакет не пришел, его время больше текущего
        if (current_time + epsilon < packet.get_scheduled_at())
        {
            // Конец перебора при встрече первого непришедшего пакета
            break;
        }

        // Если пакет уже был доступен по времени
        if (packet.get_scheduled_at() < current_time - epsilon)
        {
            // Первый непришедший пакет учтен DRR, остальные нет
            if (packet_count > 0)
            {
                // Отметка пользователя как активного претендента на ресурсы
                tti_stats.mark_user_as_resource_candidate(
                    packet.get_user_ptr());
            }
        }

        // Удаление первого элемента для доступа к следующим
        queue.pop();
        // Сохранение во временной очереди пакета для восстановления
        tmp.push(packet);

        ++packet_count;
    }

    // Восстановление пакетов в исходной очереди из временной
    while (tmp.size() > 0)
    {
        Packet packet = tmp.front();

        // Удаление первого элемента для доступа к следующим
        tmp.pop();
        // Восстановление пакета из временной очереди
        queue.push(packet);
    }
}

/*
Подсчет статистики по результатам работы планировщика
*/
void BaseDRRScheduler::evaluate_stats()
{
    stats.scheduler_total_time =
        session.get_scheduling_end_time() - session.get_scheduling_start_time();
    stats.packet_count = session.get_processed_packet_count();

    for (size_t queue_id = 0; queue_id < scheduled_queues.size(); ++queue_id)
    {
        double queue_total_time = stats.get_queue_processing_time(queue_id) +
                                  stats.get_queue_idle_time(queue_id) +
                                  stats.get_queue_wait_time(queue_id);

        stats.set_queue_total_time(queue_id, queue_total_time);
    }
}

// Перевод размера пакета из байтов в ресурсные блоки согласно размеру полезных данных в одном RB
int BaseDRRScheduler::convert_packet_size_to_rb_number(
    User *user, int packet_size)
{
    int effective_data_size_per_rb_for_user_in_bytes =
        (StandardManager::get_modulation_scheme(
            standard_name, user->get_modulation_scheme()) *
        StandardManager::get_resource_elements_in_resource_block(
            standard_name)) / 8;

    int rb_count =
        static_cast<int>(
            std::ceil(
                static_cast<double>(packet_size) / 
                effective_data_size_per_rb_for_user_in_bytes));

    return rb_count;
}

void BaseDRRScheduler::sync_user_channels()
{
    for (auto &user_info : connected_users)
    {
        User &user = user_info.second;
        double channel_out_of_sync_for = user.get_out_of_channel_sync_for();
        double new_out_of_sync = channel_out_of_sync_for + tti_duration;

        if (std::abs(new_out_of_sync - channel_sync_interval) < epsilon)
        {
            user.set_out_of_channel_sync_for(0);
            user.set_modulation_scheme(base_modulation_scheme);
        }
        else
        {
            user.set_out_of_channel_sync_for(new_out_of_sync);
        }
    }
}

IterationStats &BaseDRRScheduler::get_stats()
{
    return this->stats;
}