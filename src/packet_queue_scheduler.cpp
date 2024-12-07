#include <string>
#include "packet_queue_scheduler.hpp"

/*
Логика работы планировщика
*/
void PacketQueueScheduler::run()
{
    // Метка времени в момент запуска планировщика
    double scheduling_start = 0.0;
    double current_time = scheduling_start;
    int processed_packets_count = 0; // Счетчик обслуженных пакетов

    // Цикл до обслуживания всех пакетов во всех очередях
    while (processed_packets_count < this->total_packets)
    {
        int queue_id = 0;

        for (auto &queue : scheduled_queues)
        {
            double queue_processing_start_time = current_time;

            // Перебор пакетов только в наполненной очереди
            while (queue.size() > 0)
            {
                // Обслуживание первого в очереди пакета
                Packet packet = queue.front();
                double packet_size = packet.get_size();
                double queue_deficit = queue.get_deficit();
                double packet_scheduled_at = packet.get_scheduled_at();

                if (current_time + epsilon < packet_scheduled_at)
                {
                    break;
                }
                else
                {
                    queue.set_deficit(queue.get_deficit() + queue.get_quant());
                }

                // Проверка достаточности дефицита на обслуживание пакета
                if (packet_size <= queue_deficit + epsilon)
                {
                    // Отправка пакета на обслуживание...

                    if (queue.front().get_retry())
                    {
                        stats.retried_packet_count++;
                    }

                    queue.pop();
                    ++processed_packets_count;

                    double packet_processing_duration = packet_size;
                    double packet_delay_duration = current_time - packet_scheduled_at;


                    stats.total_processing_time += packet_processing_duration;
                    stats.queue_stats[queue_id].emplace_back(
                            packet_scheduled_at,
                            packet_delay_duration);

                    // Вычисление фактического дефицита по результатам времени обслуживания
                    queue.set_deficit(queue_deficit - packet_processing_duration);
                    current_time += packet_processing_duration;
                }
                else
                {
                    // Принудительная смена очереди из-за нехватки накопленного дефицита
                    Packet packet = queue.front();
                    packet.set_retry();
                    queue.pop();
                    queue.push(packet);
                    stats.total_skip_time += queue_switch_time;
                    break;
                }
            }
            double queue_processing_end_time = current_time;
            double queue_processing_duration = 
                queue_processing_end_time - queue_processing_start_time;
            
            stats.processing_time_by_queue[queue_id] += queue_processing_duration;

            queue_id++;
            current_time += queue_switch_time;
        }

        current_time += queue_switch_time;
    }

    // Метка времени в момент завершения работы планировщика
    double scheduling_end = current_time;
    double scheduling_duration = scheduling_end - scheduling_start;

    // Подсчет статистики
    stats.total_time = scheduling_duration;
    stats.packet_count = total_packets;

    for (auto &queue : scheduled_queues)
    {
        stats.lost_packet_count += queue.get_lost_packet_count();
    }
}

/*
Планирование очереди через запись в массив очередей
и вычисление новой суммы общего количества пакетов
*/
void PacketQueueScheduler::schedule(PacketQueue &&packet_queue)
{
    scheduled_queues.push_back(packet_queue);
    total_packets += packet_queue.size();
}

ExecutionStats& PacketQueueScheduler::get_stats()
{
    return this->stats;
}