#Matthew Dupont and Louis Finney
#R Exercises Assignment
#2021-03-05

library(tidyverse)

sequential.df <- read.csv("sequential.csv")
sequential.df <- sequential.df %>% rename(MaxTime_s=MaxTime)

chunk.df <- read.csv("chunk.csv")
chunk.df <- chunk.df  %>% rename(MaxTime_c=MaxTime)

roundRobin.df <- read.csv("roundRobin.csv")
roundRobin.df <- roundRobin.df  %>% rename(MaxTime_rr=MaxTime)

aggregated.df <- chunk.df %>% 
                 inner_join(roundRobin.df) %>% 
                 inner_join(sequential.df, by=c("Terms" = "Terms", 
                                                "Variable" = "Variable", 
                                                "Total"="Total"))

aggregated.df <- (subset(aggregated.df, select=-c(Procs.y))) %>%
                 rename(Procs=Procs.x)
                 
aggregated.df$Speedup_c = aggregated.df$MaxTime_c/aggregated.df$MaxTime_s
aggregated.df$Speedup_rr = aggregated.df$MaxTime_rr/aggregated.df$MaxTime_s

aggregated.df.50000 <- aggregated.df %>% filter(Terms == 50000)
aggregated.df.75000 <- aggregated.df %>% filter(Terms == 75000)
aggregated.df.100000 <- aggregated.df %>% filter(Terms == 100000)

ggplot(data=aggregated.df.50000, aes(x=Procs, y=Speedup_c)) +
  geom_point() +
  labs(title = "Chunked - 50000 terms", x="# Processes", y="Speedup")
ggplot(data=aggregated.df.75000, aes(x=Procs, y=Speedup_c)) +
  geom_point() +
  labs(title = "Chunked - 75000 terms", x="# Processes", y="Speedup")
ggplot(data=aggregated.df.100000, aes(x=Procs, y=Speedup_c)) +
  geom_point() +
  labs(title = "Chunked - 100000 terms", x="# Processes", y="Speedup")
ggplot(data=aggregated.df.50000, aes(x=Procs, y=Speedup_rr)) +
  geom_point() +
  labs(title = "Round Robin - 50000 terms", x="# Processes", y="Speedup")
ggplot(data=aggregated.df.75000, aes(x=Procs, y=Speedup_rr)) +
  geom_point() +
  labs(title = "Round Robin - 75000 terms", x="# Processes", y="Speedup")
ggplot(data=aggregated.df.100000, aes(x=Procs, y=Speedup_rr)) +
  geom_point() +
  labs(title = "Round Robin - 100000 terms", x="# Processes", y="Speedup")

ggplot(data=aggregated.df, aes(x=Procs, y=Speedup_c, color=factor(Terms))) +
  geom_point() +
  labs(title = "Chunked - All terms", x="# Processes", y="Speedup")

ggplot(data=aggregated.df, aes(x=Procs, y=Speedup_rr, color=factor(Terms))) +
  geom_point() +
  labs(title = "RoundRobin - All terms", x="# Processes", y="Speedup")

ggplot(data=aggregated.df, aes(x=Procs, y=MaxTime_c, color=factor(Terms))) +
  geom_point() +
  labs(title = "Chunked - All terms", x="# Processes", y="MaxTime")

ggplot(data=aggregated.df, aes(x=Procs, y=MaxTime_rr, color=factor(Terms))) +
  geom_point() +
  labs(title = "RoundRobin - All terms", x="# Processes", y="MaxTime")
