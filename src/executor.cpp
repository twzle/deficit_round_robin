#include "executor.hpp"
#include "packet_queue_scheduler.hpp"
#include "time_generator.hpp"

Executor::Executor(Settings settings){
    this->settings = settings;
}

void Executor::run(){
    for (int i = 0; i < settings.get_launches(); ++i){
        TimeGenerator::synchronize_time();

        std::cout << "Running scheduler #" << i + 1 << "...\n";
        this->execute();
    }
}

void Executor::execute(){

    std::vector<PacketQueue> queues;
    PacketQueueScheduler pqs;
            
    // Наполнение очередей пакетами
    for (int i = 0; i < settings.get_queue_count(); ++i){
        PacketQueue queue(
            settings.get_queue_quant(), 
            settings.get_queue_limit()
        );

        for (int i = 0; i < settings.get_packet_count(); ++i){
            Packet packet(settings.get_packet_size());
            queue.add_packet(packet);
        }

        // Планирование обслуживания очереди
        pqs.schedule(queue);
    }

    // Запуск планировщика
    pqs.run();

    ExecutionStats stats = pqs.get_stats();
    stats.evaluate();

    // Запись статистики текущего запуска в массив
    this->average_stats.stats_array.push_back(stats);
}

AverageStats Executor::get_stats(){
    return this->average_stats;
}