#pragma once

#include <stdexcept>
#include <memory>
#include "scheduling/base_drr_scheduler.hpp"

class Settings {
    public:
        Settings() = default;
        Settings(
            int launches,
            std::string standard_type,
            std::string tti_duration,
            std::string scheduler_type,
            double bandwidth, 
            int packet_count, 
            int packet_size, 
            int queue_count, 
            int queue_quant,
            int queue_limit,
            double time_lambda);

        void validate();

        int get_launches();
        std::string get_standard_type();
        std::string get_tti_duration();
        std::unique_ptr<BaseDRRScheduler> get_scheduler_instance();
        double get_bandwidth();
        int get_packet_count();
        int get_packet_size();
        int get_queue_count();
        int get_queue_quant();
        int get_queue_limit();
        double get_time_lambda();
        int get_resource_block_per_tti_limit();

    private:
        int launches; // Количество повторов
        std::string standard_type; // Стандарт передачи данных
        std::string tti_duration; // Длительность TTI
        std::string scheduler_type; // Тип планировщика
        double bandwidth; // Полоса пропускания в МГц
        int packet_count; // Количество пакетов для каждого повтора
        int packet_size; // Размеры пакетов для каждого повтора
        int queue_count; // Количество очередей для каждого повтора
        int queue_quant; // Квант времени очередей для каждого повтора
        int queue_limit; // Размеры очередей для каждого повтора
        double time_lambda; // Частота прихода пакетов в единицу времени
};