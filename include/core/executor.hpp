#pragma once

#include "settings.hpp"
#include "stats/mean_stats.hpp"

class Executor
{
    public:
        Executor(Settings settings);
        void run();
        void execute();
        MeanStats& get_stats();
    
    private:
        Settings settings; // Конфигурация запуска
        MeanStats mean_stats; // Статистика по всем запускам планировщика
};