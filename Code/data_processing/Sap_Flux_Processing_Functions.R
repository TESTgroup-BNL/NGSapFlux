#### Load required libraries, clear workspace, set wd and file paths  #####
library(tidyverse)
library(lubridate)
library(plyr)
library(hms)
library(stats)
library(rqdatatable)

rm(list=ls())
setwd("~/Desktop")
Path_A <- "~/Google Drive/TESTgroup/Projects/BRF-SBU/SapFlux/Data/BoxA" #file path to data source, using BRF data for testing

#### Functions ####

# Converts from resistance to temp in deg C 
convert_temp <- function(x){
  Temp_C <- 97.1383 - 236.7707*x + 262.5395*x^2 - 155.3591*x^3
  return(Temp_C)
} 

# Helper function to find mode value to establish a pre-heat baseline temp for QAQC and data processing
getmode <- function(v) {
  uniqv <- unique(v)
  uniqv[which.max(tabulate(match(v, uniqv)))]
} 

# Set up to run for each CR1000 box's data in a separate directory. Can be changed to process all boxes from one directory preferred
load_Data <- function(filepath, Origin_DateTime){
  ls_files <- list.files(path=filepath, full.names = T) 
  Header <- read.csv(file=ls_files[1], header = F, skip=1,nrows = 1, as.is = T)
  BoxA=do.call("rbind.fill", apply(X = as.matrix(ls_files),FUN = read.csv,MARGIN = 1, skip=4,header=F))
  colnames(BoxA) <- Header
  names(BoxA) <- c("TIMESTAMP", "RECORD", "TREE1_TH1", "TREE2_TH1", "TREE3_TH1", "TREE4_TH1", "TREE5_TH1",
                   "TREE1_TH2", "TREE2_TH2", "TREE3_TH2", "TREE4_TH2", "TREE5_TH2", "TREE1_TH3", "TREE2_TH3", 
                   "TREE3_TH3", "TREE4_TH3", "TREE5_TH3", "HT_START") # stock names for sapflux data frames
  to_convert <- grep("TREE", names(BoxA))
  BoxA$TIMESTAMP <- ymd_hms(BoxA$TIMESTAMP, tz="UTC")
  BoxA$Date <- date(BoxA$TIMESTAMP)
  BoxA$Time <- as_hms(BoxA$TIMESTAMP)
  BoxA <- BoxA[BoxA$TIMESTAMP >=Origin_DateTime,]
  BoxA <- BoxA[complete.cases(BoxA), ]
  BoxA[to_convert] <- lapply(BoxA[to_convert], as.numeric)
  BoxA[to_convert] <- lapply(BoxA[to_convert], convert_temp)
  BoxA$Min <- minute(BoxA$TIMESTAMP)
  BoxA$Baseline <- BoxA$Min %in% c(0,1,2,30,31,32)
  BoxA$Heat <- BoxA$Min %in% c(3,4,33,34)
  BoxA$Pulse <- as.factor(paste0(hour(BoxA$Time),"_",minute(round_date(BoxA$TIMESTAMP, "30 minutes"))))
  levels(BoxA$Pulse) <- c('P01','P02','P03','P04','P21','P22','P23','P24','P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38',
                          'P39','P40','P05','P06','P41','P42','P43','P44','P45','P46','P47','P48','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20')
  return(BoxA)
  
}

# Course QAQC processing. Identifies outliers and flags if data are messy by time period (e.g., 12:00:00-12:01:59) based on number of errant observations. Also detects if heater fired. Data returned as TRUE indicates an error 
# This function is VERY SLOW, can be run in parallel to speed up
Sanity_Check_Corse <- function(df,Dates=unique(df$Date),Pre_Threshold_Temp=0.5,Pre_Threshold_Num=20, Post_Threshold_Temp=5, Post_Threshold_Num=10, HT_Temp=0.5){
  QC_Pre <- data.frame(Date= rep(as.character(Dates), each=48), 
                       Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                    'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                       TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  QC_Post <- data.frame(Date= rep(as.character(Dates), each=48), 
                        Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                     'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                        TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  QC_HT <- data.frame(Date= rep(as.character(Dates), each=48), 
                      Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                   'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                      HT1=NA,HT2=NA,HT3=NA,HT4=NA,HT5=NA)
  
  for (j in as.character(Dates)){
    for (k in c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48')){
      MODE_Pre <- sapply(df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17],FUN=getmode)
      MODE_Post <- sapply(df[df$Date==j &df$Pulse==k&df$Baseline==F,3:17],FUN=getmode)
      MODE_HT <- sapply(df[df$Date==j &df$Pulse==k&df$Heat==T,8:12],FUN=max)
      if(is.na(MODE_Pre)[1]){
        QC_Pre[QC_Pre$Date==j &QC_Pre$Pulse==k,3:17] <- TRUE
        QC_Post[QC_Post$Date==j &QC_Post$Pulse==k,3:17]  <- TRUE
        QC_HT[QC_HT$Date==j &QC_HT$Pulse==k,3:7] <- TRUE
      }else{
        QC_Pre_x <- df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17]>= matrix(rep(t(data.frame(MODE_Pre+Pre_Threshold_Temp)),180), nrow=180,byrow = T)|df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17]<=matrix(rep(t(data.frame(MODE_Pre-Pre_Threshold_Temp)),180), nrow=180,byrow = T)
        QC_Pre[QC_Pre$Date==j &QC_Pre$Pulse==k,3:17]  <- colSums(QC_Pre_x, na.rm = TRUE)>=Pre_Threshold_Num
        QC_Post_x<- df[df$Date==j &df$Pulse==k&df$Baseline==F,3:17]>=matrix(data= rep(MODE_Post+Post_Threshold_Temp,720),ncol=15, byrow=T)|df[df$Date==j &df$Pulse==k&df$Baseline==F,3:17]<=matrix(data= rep(MODE_Post-Post_Threshold_Temp,720),ncol=15, byrow=T)
        QC_Post[QC_Post$Date==j &QC_Post$Pulse==k,3:17]  <- colSums(QC_Post_x, na.rm = TRUE)>=Post_Threshold_Num
        QC_HT[QC_HT$Date==j &QC_HT$Pulse==k,3:7] <-  as.logical(MODE_HT-MODE_Pre[6:10]<=HT_Temp)
      }
    }}
  QAQC <- list(QC_Pre,QC_Post,QC_HT)
  return(QAQC)
} 

# Fine QAQC processing. Identifies outliers and flags if data are messy by individual point. Data returned as TRUE indicates an error 
# This function is VERY SLOW, can be run in parallel to speed up
Sanity_Check_Fine <- function(df,Dates=unique(df$Date),Pre_Threshold_Temp=0.5, Post_Threshold_Temp=5){
  QC_Pre <- data.frame(Date= rep(as.character(Dates), each=48*180), 
                       Pulse= rep(rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                        'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'),each=180), times=length(Dates)),
                       TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  QC_Post <- data.frame(Date= rep(as.character(Dates), each=48*720), 
                        Pulse= rep(rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                         'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'),each=720), times=length(Dates)),
                        TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  
  for (j in as.character(Dates)){
    for (k in c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48')){
      MODE_Pre <- sapply(df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17],FUN=getmode)
      MODE_Post <- sapply(df[df$Date==j &df$Pulse==k&df$Baseline==F,3:17],FUN=getmode)
      if(is.na(MODE_Pre)[1]){
        QC_Pre[QC_Pre$Date==j &QC_Pre$Pulse==k,3:17] <- TRUE
        QC_Post[QC_Post$Date==j &QC_Post$Pulse==k,3:17]  <- TRUE
        
      }else{
        QC_Pre[QC_Pre$Date==j &QC_Pre$Pulse==k,3:17] <- df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17]>= matrix(rep(t(data.frame(MODE_Pre+Pre_Threshold_Temp)),180), nrow=180,byrow = T)|df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17]<=matrix(rep(t(data.frame(MODE_Pre-Pre_Threshold_Temp)),180), nrow=180,byrow = T)
        QC_Post[QC_Post$Date==j &QC_Post$Pulse==k,3:17]  <- df[df$Date==j &df$Pulse==k&df$Baseline==F,3:17]>=matrix(data= rep(MODE_Post+Post_Threshold_Temp,720),ncol=15, byrow=T)|df[df$Date==j &df$Pulse==k&df$Baseline==F,3:17]<=matrix(data= rep(MODE_Post-Post_Threshold_Temp,720),ncol=15, byrow=T)
      }
    }}
  QAQC <- list(QC_Pre,QC_Post)
  return(QAQC)
} 

# Not working yet.... Need to figure out how to replace values in df with NA when values in Pre_QAQC_df are TRUE
QAQC_Remove <- function(df,Pre_QAQC_df,Post_QAQC_df){}

# Helper function for Heat_Dis_Spline
Spline_Fit <- function(Y_vals,X_vals=Z$TIMESTAMP, X_Vals_New=X){
  Z1 <- smooth.spline(X_vals,Y_vals)
  Z2 <- predict(Z1, X_Vals_New)
  return(Z2$y)
  
}

# Finds the baseline temp before the heat pulse, the average increase in temperature from the baseline for a given period to time (30-180 sec post heat by default), the time of peak heating, and the time at which the heat returns to baseline
# This function requires removal of outlier points in the preheat phase. Otherwise the spline will not work
Heat_Dis_Spline <- function(df,Dates=unique(df$Date),Time_Post_Heat=c(30:180)){
  Baseline <- data.frame(Date= rep(as.character(Dates), each=48), 
                         Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                      'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                         TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  Heat_time <- data.frame(Date= rep(as.character(Dates), each=48), 
                          Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                       'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                          TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  Heat_Increase <- data.frame(Date= rep(as.character(Dates), each=48), 
                              Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                           'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                              TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  Cool_time <- data.frame(Date= rep(as.character(Dates), each=48), 
                          Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                       'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                          TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  
  for (j in as_date(Dates)){
    Z <- df[df$TIMESTAMP>=ymd_hms(paste0(as_date(j)-1," 12:00:00"),tz="UTC")& df$TIMESTAMP<=ymd_hms(paste0(as_date(j)+1," 11:59:59"),tz="UTC")&df$Baseline==T, ]
    Z <- Z[complete.cases(Z[,1:17]),]
    Z$TIMESTAMP <- as.numeric(Z$TIMESTAMP)
    X <- df[df$Date== as_date(j) & df$Baseline==F&df$Min %in% c(14,44), ]
    X$Sec <- second(X$TIMESTAMP)
    X <- as.numeric(X[X$Sec==0, "TIMESTAMP"])
    Spline_Fit <- function(Y_vals,X_vals=Z$TIMESTAMP, X_Vals_New=X){
      Z1 <- smooth.spline(X_vals,Y_vals)
      Z2 <- predict(Z1, X_Vals_New)
      return(Z2$y)
    }
    
    Baseline[Baseline$Date==as_date(j),3:17] <- sapply(Z[,3:17], Spline_Fit)
    
    for (k in c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48')){
      Z <- df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,3:17][Time_Post_Heat,]
      X <- matrix(data=unlist(rep(Baseline[Baseline$Date==as_date(j)&Baseline$Pulse==k,3:17], length(Time_Post_Heat))),ncol=15,byrow = T)
      Heat_Increase[Heat_Increase$Date==as_date(j)&Heat_Increase$Pulse==k, 3:17] <- colMeans(Z- X)
      
      X <- sapply(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,3:17],which.max)
      Heat_time[Heat_time$Date==as_date(j)&Heat_time$Pulse==k, 3:17] <- as.character(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,"TIMESTAMP"][X])
      
      Y1 <- c()
      for (i in 3:17){ 
        Y <- which(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,i]>=Baseline[Baseline$Date==as_date(j)&Baseline$Pulse==k,i])
        if(identical(Y, integer(0))){
          Y <- 720
        }else{
          Y <-Y[1]}
        Y1 <- c(Y1,Y)}
      
      Cool_time[Cool_time$Date==as_date(j)&Cool_time$Pulse==k, 3:17] <- as.character(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,"TIMESTAMP"][Y1])
    }
  }
  
  Heat_Dis_Data <- list(Baseline,Heat_Increase,Heat_time,Cool_time)
  return(Heat_Dis_Data)
} 

# Finds the baseline temp before the heat pulse, the average increase in temperature from the baseline for a given period to time (30-180 sec post heat by default), the time of peak heating, and the time at which the heat returns to baseline
# This function is robust to outlier points, but may not capture diurnal heating trends (i.e., assumes baseline is the same for pre and post heat)
Heat_Dis_Mode <- function(df,Dates=unique(df$Date),Time_Post_Heat=c(30:180)){
  Baseline <- data.frame(Date= rep(as.character(Dates), each=48), 
                         Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                      'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                         TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  Heat_time <- data.frame(Date= rep(as.character(Dates), each=48), 
                          Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                       'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                          TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  Heat_Increase <- data.frame(Date= rep(as.character(Dates), each=48), 
                              Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                           'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                              TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  Cool_time <- data.frame(Date= rep(as.character(Dates), each=48), 
                          Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                       'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                          TREE1_TH1=NA,TREE2_TH1=NA,TREE3_TH1=NA,TREE4_TH1=NA,TREE5_TH1=NA,TREE1_TH2=NA,TREE2_TH2=NA,TREE3_TH2=NA,TREE4_TH2=NA,TREE5_TH2=NA,TREE1_TH3=NA,TREE2_TH3=NA,TREE3_TH3=NA,TREE4_TH3=NA,TREE5_TH3=NA)
  
  for (j in as_date(Dates)){
    for (k in c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48')){
      Baseline[Baseline$Date==as_date(j)&Baseline$Pulse==k,3:17] <- sapply(df[df$Date==j &df$Pulse==k&df$Baseline==T,3:17],FUN=getmode)
      Z <- df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,3:17][Time_Post_Heat,]
      X <- matrix(data=unlist(rep(Baseline[Baseline$Date==as_date(j)&Baseline$Pulse==k,3:17], length(Time_Post_Heat))),ncol=15,byrow = T)
      Heat_Increase[Heat_Increase$Date==as_date(j)&Heat_Increase$Pulse==k, 3:17] <- colMeans(Z- X)
      
      
      
      X <- sapply(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,3:17],which.max)
      Heat_time[Heat_time$Date==as_date(j)&Heat_time$Pulse==k, 3:17] <- as.character(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,"TIMESTAMP"][X])
      
      Y1 <- c()
      for (i in 3:17){ 
        Y <- which(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,i]<=Baseline[Baseline$Date==as_date(j)&Baseline$Pulse==k,i])
        if(identical(Y, integer(0))){
          Y <- 720
        }else{
          Y <-Y[1]}
        Y1 <- c(Y1,Y)}
      
      Cool_time[Cool_time$Date==as_date(j)&Cool_time$Pulse==k, 3:17] <- as.character(df[df$Date==as_date(j)& df$Pulse==k&df$Baseline==F,"TIMESTAMP"][Y1])
    }
  }
  
  Heat_Dis_Data <- list(Baseline,Heat_Increase,Heat_time,Cool_time)
  return(Heat_Dis_Data)
} 

# Solves the HRM equations for sensors 1 and 2. K can either as a vector or assumed
Solve_HRM <- function(df,Dates=unique(df$Date),K=rep(0.0025,5)){
  HRM <- data.frame(Date= rep(as.character(Dates), each=48), 
                    Pulse= rep(c('P01','P02','P03','P04','P05','P06','P07','P08','P09','P10','P11','P12','P13','P14','P15','P16','P17','P18','P19','P20','P21','P22','P23','P24',
                                 'P25','P26','P27','P28','P29','P30','P31','P32','P33','P34','P35','P36','P37','P38','P39','P40','P41','P42','P43','P44','P45','P46','P47','P48'), times=length(Dates)),
                    TREE1=NA,TREE2=NA,TREE3=NA,TREE4=NA,TREE5=NA)
  
  HRM$TREE1 <- K[1]/0.2*log(df$TREE1_TH2/df$TREE1_TH1)*3600
  HRM$TREE2 <- K[2]/0.2*log(df$TREE2_TH2/df$TREE2_TH1)*3600
  HRM$TREE3 <- K[3]/0.2*log(df$TREE3_TH2/df$TREE3_TH1)*3600
  HRM$TREE4 <- K[4]/0.2*log(df$TREE4_TH2/df$TREE4_TH1)*3600
  HRM$TREE5 <- K[5]/0.2*log(df$TREE5_TH2/df$TREE5_TH1)*3600
  
  return(HRM)
} 

