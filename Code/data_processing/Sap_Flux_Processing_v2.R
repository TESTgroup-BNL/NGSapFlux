# Load packages
list.of.packages <- c("dplyr","ggplot2","lubridate","plyr","hms","stats","here")
new.packages <- list.of.packages[!(list.of.packages %in% installed.packages()[,"Package"])]
if(length(new.packages)) install.packages(new.packages, dependencies=TRUE)
# load
invisible(lapply(list.of.packages, library, character.only = TRUE))

# clean up first
rm(list=ls())

# Load functions
here::here()
source(paste0(file.path(here::here(),"Code","data_processing"),"/sapflux_functions.R"))

# set paths - make sure you open the script from within the git rep structure
here::here()
# This should show something like below saying you are in the NGSapFlux folder
# > here::here()
# [1] "/Users/sserbin/Data/GitHub/NGSapFlux" 
#

#Path_A <- file.path(here::here(),"Code/data_processing/SapFlux_Data/AL")
Path_A <- file.path(here::here(),"Code/data_processing/SapFlux_Data/AL/3-1")
Path_B <- file.path(here::here(),"Code/data_processing/SapFlux_Data/BRF")


# Create example output
#output_dir <- "tempdir"
output_dir <- file.path(here::here(),"Code/data_processing/SapFlux_Data/example_output")
if (output_dir=="tempdir") {
  outdir <- tempdir()
} else {
  if (! file.exists(output_dir)) dir.create(output_dir,recursive=TRUE)
  outdir <- file.path(path.expand(output_dir))
}
setwd(outdir) # set working directory
getwd()  # check wd

#### Example runs using data from BRF  #####
BoxB <- load_Data(Path_B,Origin_DateTime="2022-10-04 00:00:00",End_DateTime = "2022-10-04 23:59:59")

ggplot(BoxB, aes(Time, TREE1_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE1_TH2,color="middle"))+geom_point(aes(Time, TREE1_TH3,color="top"))+ggtitle("Tree 1")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB, aes(Time, TREE2_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE2_TH2,color="middle"))+geom_point(aes(Time, TREE2_TH3,color="top"))+ggtitle("Tree 2")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB, aes(Time, TREE3_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE3_TH2,color="middle"))+geom_point(aes(Time, TREE3_TH3,color="top"))+ggtitle("Tree 3")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB, aes(Time, TREE4_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE4_TH2,color="middle"))+geom_point(aes(Time, TREE4_TH3,color="top"))+ggtitle("Tree 4")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB, aes(Time, TREE5_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE5_TH2,color="middle"))+geom_point(aes(Time, TREE5_TH3,color="top"))+ggtitle("Tree 5")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))

A <- Sanity_Check_Corse(BoxB)
QC_Pre <- A[[1]]
QC_Post <- A[[2]]
QC_HT <- A[[3]]

B <- Sanity_Check_Fine(BoxB)
QC_Pre_Fine <- B[[1]]
QC_Post_Fine <- B[[2]]

BoxB_QAQC <- QAQC_Remove(BoxB,QC_Pre_Fine,QC_Post_Fine)

ggplot(BoxB_QAQC, aes(Time, TREE1_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE1_TH2,color="middle"))+geom_point(aes(Time, TREE1_TH3,color="top"))+ggtitle("Tree 1")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB_QAQC, aes(Time, TREE2_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE2_TH2,color="middle"))+geom_point(aes(Time, TREE2_TH3,color="top"))+ggtitle("Tree 2")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB_QAQC, aes(Time, TREE3_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE3_TH2,color="middle"))+geom_point(aes(Time, TREE3_TH3,color="top"))+ggtitle("Tree 3")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB_QAQC, aes(Time, TREE4_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE4_TH2,color="middle"))+geom_point(aes(Time, TREE4_TH3,color="top"))+ggtitle("Tree 4")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxB_QAQC, aes(Time, TREE5_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE5_TH2,color="middle"))+geom_point(aes(Time, TREE5_TH3,color="top"))+ggtitle("Tree 5")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))

C <- Heat_Dis_Spline(BoxB_QAQC)
Baseline <- C[[1]]
Heat_Increase_spline <- C[[2]]
Heat_Pulse <- C[[3]]
Cool_Time <- C[[4]]

D <- Heat_Dis_Mode(BoxB_QAQC)
Baseline <- D[[1]]
Heat_Increase <- D[[2]]
Heat_Pulse <- D[[3]]
Cool_Time <- D[[4]]



E <- Solve_HRM(Heat_Increase_spline)
ggplot(E, aes(Pulse,TREE3))+geom_point()+ylab("Vh; cm hr-1")
ggplot(E, aes(Pulse,TREE4))+geom_point()+ylab("Vh; cm hr-1")
ggplot(E, aes(Pulse,TREE5))+geom_point()+ylab("Vh; cm hr-1")


.F <- Solve_HRM(Heat_Increase)
ggplot(.F, aes(Pulse,TREE3))+geom_point()+ylab("Vh; cm hr-1")
ggplot(.F, aes(Pulse,TREE4))+geom_point()+ylab("Vh; cm hr-1")
ggplot(.F, aes(Pulse,TREE5))+geom_point()+ylab("Vh; cm hr-1")





#### Example runs using data from AL  #####
#BoxA <- load_Data(Path_A)
#BoxA <- load_Data(Path_A,Origin_DateTime="2022-06-15 00:00:00",End_DateTime = "2022-06-15 16:00:00")
#BoxA <- load_Data(Path_A,Origin_DateTime="2022-07-20 00:00:00",End_DateTime = "2022-07-20 23:59:59")
BoxA <- load_Data(Path_A, Origin_DateTime="2022-09-07 00:00:00", End_DateTime = "2022-09-07 23:59:59")

BoxA <- load_Data(Path_A, Origin_DateTime="2022-09-08 00:00:00", End_DateTime = "2022-09-08 23:59:59")

summary(BoxA)

ggplot(BoxA, aes(Time, TREE1_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE1_TH2,color="middle"))+geom_point(aes(Time, TREE1_TH3,color="top"))+ggtitle("Tree 1")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA, aes(Time, TREE2_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE2_TH2,color="middle"))+geom_point(aes(Time, TREE2_TH3,color="top"))+ggtitle("Tree 2")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA, aes(Time, TREE3_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE3_TH2,color="middle"))+geom_point(aes(Time, TREE3_TH3,color="top"))+ggtitle("Tree 3")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA, aes(Time, TREE4_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE4_TH2,color="middle"))+geom_point(aes(Time, TREE4_TH3,color="top"))+ggtitle("Tree 4")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA, aes(Time, TREE5_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE5_TH2,color="middle"))+geom_point(aes(Time, TREE5_TH3,color="top"))+ggtitle("Tree 5")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))


A <- Sanity_Check_Corse(BoxA)
QC_Pre <- A[[1]]
QC_Post <- A[[2]]
QC_HT <- A[[3]]

B <- Sanity_Check_Fine(BoxA)
QC_Pre_Fine <- B[[1]]
QC_Post_Fine <- B[[2]]

BoxA_QAQC <- QAQC_Remove(BoxA,QC_Pre_Fine,QC_Post_Fine)

ggplot(BoxA_QAQC, aes(Time, TREE1_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE1_TH2,color="middle"))+geom_point(aes(Time, TREE1_TH3,color="top"))+ggtitle("Tree 1")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA_QAQC, aes(Time, TREE2_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE2_TH2,color="middle"))+geom_point(aes(Time, TREE2_TH3,color="top"))+ggtitle("Tree 2")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA_QAQC, aes(Time, TREE3_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE3_TH2,color="middle"))+geom_point(aes(Time, TREE3_TH3,color="top"))+ggtitle("Tree 3")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA_QAQC, aes(Time, TREE4_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE4_TH2,color="middle"))+geom_point(aes(Time, TREE4_TH3,color="top"))+ggtitle("Tree 4")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))
ggplot(BoxA_QAQC, aes(Time, TREE5_TH1))+ geom_point(aes(color="bottom"))+geom_point(aes(Time, TREE5_TH2,color="middle"))+geom_point(aes(Time, TREE5_TH3,color="top"))+ggtitle("Tree 5")+
  scale_color_manual(name = "sensor",values = c( "top" = "blue", "middle" = "red", "bottom" = "black"),
                     labels = c("top", "middle", "bottom"))

C <- Heat_Dis_Spline(BoxA_QAQC)
Baseline <- C[[1]]
Heat_Increase_spline <- C[[2]]
Heat_Pulse <- C[[3]]
Cool_Time <- C[[4]]

D <- Heat_Dis_Mode(BoxA_QAQC)
Baseline <- D[[1]]
Heat_Increase <- D[[2]]
Heat_Pulse <- D[[3]]
Cool_Time <- D[[4]]


E <- Solve_HRM(Heat_Increase_spline)
ggplot(E, aes(Pulse,TREE1))+geom_point()+ylab("Vh; cm hr-1")
ggplot(E, aes(Pulse,TREE2))+geom_point()+ylab("Vh; cm hr-1")
ggplot(E, aes(Pulse,TREE3))+geom_point()+ylab("Vh; cm hr-1")
ggplot(E, aes(Pulse,TREE4))+geom_point()+ylab("Vh; cm hr-1")
ggplot(E, aes(Pulse,TREE5))+geom_point()+ylab("Vh; cm hr-1")


F <- Solve_HRM(Heat_Increase)
ggplot(F, aes(Pulse,TREE1))+geom_point()+ylab("Vh; cm hr-1")
ggplot(F, aes(Pulse,TREE2))+geom_point()+ylab("Vh; cm hr-1")
ggplot(F, aes(Pulse,TREE3))+geom_point()+ylab("Vh; cm hr-1")
ggplot(F, aes(Pulse,TREE4))+geom_point()+ylab("Vh; cm hr-1")
ggplot(F, aes(Pulse,TREE5))+geom_point()+ylab("Vh; cm hr-1")

write.csv(x = F, file = file.path(outdir,"test_F_1.csv"))


