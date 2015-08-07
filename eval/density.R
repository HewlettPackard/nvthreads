#Read csv files and calculate the amount of writes per dirty page
#Indrajit Roy (Aug., 2015)
##########################

findDensity<-function(fname){

 print(paste("Processing file:",fname,sep=""))
 input<-read.csv(file=fname, skip=0, header=FALSE)
 byteCol<-as.numeric(input$V1)
 pageCol<-as.numeric(input$V2)
 total_pages<-sum(pageCol) 
 bytes_written<-sum(byteCol*pageCol)
 density<-(bytes_written*100)/(4096*total_pages)
 dirty25<-(sum(pageCol[1:1024])*100)/total_pages #Pages that are <25% full
 dirty50<-(sum(pageCol[1:2048])*100)/total_pages #Pages that are <50% full
 dirty75<-(sum(pageCol[1:3072])*100)/total_pages #Pages that are <75% full

 #cat("Total pages=",total_pages,": Bytes written=",bytes_written,": Density(%)=",density,"\n")
 #cat("Pages <25% dirty(%)=",dirty25,": Pages <50% dirty(%)=",dirty50,": Pages <75% dirty(%)=",dirty75,"\n")
 ret<-data.frame(app=basename(fname), pages=total_pages,bytes_MB=bytes_written/(1024*1024), density=density, dirty25=dirty25, dirty50=dirty50, dirty75=dirty75)
 return (ret)
}


#Read files in a given directory
inputDir<-getwd()
args<-(commandArgs(TRUE))
cat("**************************************\n")
cat("Usage: RScript density.R [path-to-dir-with-csv-files or csv-file]\n")
cat("Example: RScript density.R dir1 \n")
cat("**************************************\n")
if(length(args)==0){
  cat("No arguments passed. Will use current directory for input\n")
}else {
  inputDir<-args[[1]]
} 
print(paste("Reading files in directory", inputDir, sep=""))
dirFiles<-NULL
if(file.info(inputDir)$isdir){
  dirFiles<-list.files(inputDir, pattern="*.csv", full.names=TRUE)
}else{
  #Looks like this is just a file
  dirFiles<-inputDir
}
nfiles<-length(dirFiles)
if(nfiles==0){
  cat("No files found. Exiting\n")
}else{
  cat("Will process ",nfiles," csv files\n")
  res<-lapply(dirFiles, findDensity)
  df<-do.call(rbind, res)
  cat("**************************************\n")
  print(df)
}
