#include "global.h"
#include "hash.h"
#include "hash2.h"
#include <stdlib.h>     /* atexit */
#include <mpi.h>
#include <string.h>
#include "cstr-utils.h"
#include "ppid_check.h"

// global 
 bool peakprof_init_flag=false;
 double apptime=0.0;
 double libtime=0.0;
 double layer_time[MAX_LAYER];
 char layer_caller[MAX_LAYER][40];
 int layer_count;
 char **record_f=NULL;

// environmental variables
 int peakprof_debug=0;
 int peakprof_mkl_fake=-1;
 int peakprof_record_rank=0;
 double peakprof_record_threshold=0.000;
 char peakprof_record_function[1000];

//local
 char *argv0;
 int ifmpi;
 int flag_clean_fppid = 0;



/*
int mkl_serv_intel_cpu_true() {
   int (*fp)()=NULL;
   
   if (peakprof_mkl_fake==1) return 1;
   if (peakprof_mkl_fake==0) return 0; 
   
   fp=dlsym(RTLD_NEXT, __func__);
   return fp();
}
*/


void env_get()
{
   char* myenv;

   myenv = getenv("PEAKPROF_MKL_FAKE");
   peakprof_mkl_fake = myenv? atoi(myenv) : -1;        
  
   myenv = getenv("PEAKPROF_DEBUG");
   peakprof_debug = myenv? atoi(myenv) : 0 ;        

   myenv = getenv("PEAKPROF_RECORD_RANK");
   peakprof_record_rank = myenv? atoi(myenv) : 0 ;        

   myenv = getenv("PEAKPROF_RECORD_THRESHOLD");
   peakprof_record_threshold = myenv? atof(myenv) : 0.00 ;        

   myenv = getenv("PEAKPROF_RECORD_FUNCTION");
//  if(myenv) strcpy(peakprof_record_function , myenv) ;        
//  record_f=myenv? str_split(peakprof_record_function, ',') : NULL;
   if(myenv) { 
        strcpy(peakprof_record_function , myenv) ;        
        record_f=str_split(peakprof_record_function,',');
   // below is neccessary as str_split will chop the original string.
        strcpy(peakprof_record_function , myenv) ; 
   }  
   else  record_f=NULL; 


   return ;
}

void env_show()
{
   fprintf(OUTFILE, "environmental variables:\n");
//   fprintf(OUTFILE, "PEAKPROF_MKL_FAKE = %d \n",peakprof_mkl_fake); 
//   fprintf(OUTFILE, "    PEAKPROF_DEBUG=%d \n",peakprof_debug);
   fprintf(OUTFILE, "    PEAKPROF_RECORD_RANK=%d \n",peakprof_record_rank);
   fprintf(OUTFILE, "    PEAKPROF_RECORD_FUNCTION=%s \n",peakprof_record_function);
   fprintf(OUTFILE, "    PEAKPROF_RECORD_THRESHOLD=%.3f \n",peakprof_record_threshold);

   return ;
}

int MPI_Finalize(void) {
//    printf("--- My Final ---\n");
    return 0;
}

void  MPI_Finalize_(int *ierr) {
//    printf("--- My Final_ ---\n");
    ierr=0;
    return ;
}

/* somehow causes crash
void  mpi_finalize_f08_(int *ierr) {
    printf("--- my final_f08_ ---\n");
    ierr=0;
    return ;
}
*/

int (*original_pmpi_finalize)(void)=NULL;
int peak_done = 0;
int PMPI_Finalize(void) {
//    printf("--- My PFinal ---\n");
    if (!original_pmpi_finalize) {
        original_pmpi_finalize = dlsym(RTLD_NEXT, "PMPI_Finalize");
    }   
    return 0;
}


void print_result() {

    struct item* farray=NULL;
    int fn = hash_get_size();
    int k;
    if (fn == 0) return;   //nothing profiled
    farray=hash_to_array();

    fprintf(OUTFILE,"\n"); 
    fprintf(OUTFILE, "        ----------------------------------------------------\n");
    fprintf(OUTFILE, "                         PEAK Prof Library\n");
    fprintf(OUTFILE, "        ----------------------------------------------------\n");
    fprintf(OUTFILE, "for application: %s\n\n",argv0);
    if(ifmpi)  fprintf(OUTFILE, "recorded MPI rank: %d\n",peakprof_record_rank);
//   fprintf(OUTFILE,"----------------------------- PEAK Prof -------------------------------\n");
    fprintf(OUTFILE,"total runtime: %.3fs\nlibrary time: %.3fs\npercentage of lib: %.1f%\n\n", \
                     apptime, libtime, libtime/apptime*100);
//    printf("layer_time=%.3fs\n",layer_time[0]);
//   fprintf(OUTFILE,"-------------------------------------------------------------------------\n");
    env_show();
     // hash_show_final();

// direct call 
    qsort(farray, fn, sizeof(struct item), compare_time_di);
    fprintf(OUTFILE,"\n----------------  function statistics (direct) ---------------\n");
    fprintf(OUTFILE,"    direct call time (in seconds) and counts\n");
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"    |   group    |    function    |    count   |    time    |\n");
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    k=0;
    for (int i=0; i<fn; i++) {
       if(farray[i].value.count_di>0)
         if(farray[i].value.time_di > peakprof_record_threshold)
             fprintf(OUTFILE,"%3d | %10s | %14s | %10lu | %10.3f |\n", ++k, farray[i].value.fgroup, farray[i].key, farray[i].value.count_di, farray[i].value.time_di);
    }
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"%48s %10.3f\n","total library time:", libtime);
    fprintf(OUTFILE,"--------------------------------------------------------------\n");

// exlusive time
    qsort(farray, fn, sizeof(struct item), compare_time_ex);
    fprintf(OUTFILE,"\n--------------  function statistics (exclusive) --------------\n");
    fprintf(OUTFILE,"    exclusive call time (in seconds) and counts\n");
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"    |   group    |    function    |    count   |    time    |\n");
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    k=0;
    for (int i=0; i<fn; i++) {
         if(farray[i].value.time_ex > peakprof_record_threshold)
             fprintf(OUTFILE,"%3d | %10s | %14s | %10lu | %10.3f |\n", ++k, farray[i].value.fgroup, farray[i].key, farray[i].value.count, farray[i].value.time_ex);
    }
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"%48s %10.3f\n","total library time:", libtime);
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"\n"); 

// inclusive time
    qsort(farray, fn, sizeof(struct item), compare_time_in);
    fprintf(OUTFILE,"\n--------------  function statistics (inclusive) --------------\n");
    fprintf(OUTFILE,"    inclusive call time (in seconds) and counts\n");
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"    |   group    |    function    |    count   |    time    |\n");
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    k=0;
    for (int i=0; i<fn; i++) {
         if(farray[i].value.time_in > peakprof_record_threshold)
             fprintf(OUTFILE,"%3d | %10s | %14s | %10lu | %10.3f |\n", ++k, farray[i].value.fgroup, farray[i].key, farray[i].value.count, farray[i].value.time_in);
    }
    fprintf(OUTFILE,"--------------------------------------------------------------\n");
    fprintf(OUTFILE,"\n"); 

    hash2_show_sorted();
   
    fflush(OUTFILE); 
    free(farray);
    return;
}

void reduce_result() {
    int my_rank_id, my_rank_size;

    if (!original_pmpi_finalize) PMPI_Finalize(); //register original_pmpi_finalize

    int init_flag;
    MPI_Initialized(&init_flag);
    if(!init_flag) MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank_id);
    MPI_Comm_size(MPI_COMM_WORLD, &my_rank_size);

    struct item* farray=NULL;
    int fn = hash_get_size();
    
    farray=hash_to_array();
    int* fnarray=(int*)malloc(sizeof(int) * my_rank_size);
    MPI_Gather(&fn, 1, MPI_INT, fnarray, 1, MPI_INT, 0, MPI_COMM_WORLD);
 //   if (my_rank_id==0) 
//       for(int i=0;i<my_rank_size;i++) printf("fn[%d]=%d\n",i,fnarray[i]);

//  MPI_Reduce(values, &sum_values, NUM_COUNTERS, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
//  MPI_Reduce(values_uc, &sum_values_uc, NUM_COUNTERS_UC*SOCKETS, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // only prints designated rank. no reduction yet. 

    if (my_rank_id == peakprof_record_rank) {
        print_result();
    }
/*
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_rank_id == 63) {
        print_result();
    }
*/
    free(farray); free(fnarray);
    original_pmpi_finalize();
    return;
}

void libprof_fini(){
    apptime = mysecond()-apptime;
    if (ifmpi)  {
        reduce_result(); 
    }
    else 
        print_result();
    if(flag_clean_fppid) {
        remove_ppid_file(PPID_FILE_NAME);
    }
}

void libprof_init(){

/*  need to print in MPI_INIT in MPI, or here for serial
   fprintf(OUTFILE, "        ----------------------------------------------------\n");
   fprintf(OUTFILE, "                    Starting PEAK Prof Library\n");
   fprintf(OUTFILE, "        ----------------------------------------------------\n");
*/
    peakprof_init_flag=true;
    layer_count=0; 
    strcpy(layer_caller[0],"user");
    memset(layer_time, 0, MAX_LAYER*sizeof(layer_time[0]));
    env_get();
    get_argv0(&argv0);
    ifmpi=check_MPI();
    if (ifmpi){
        int is_parent_MPI = check_parent_process(PPID_FILE_NAME, &flag_clean_fppid);
        if(is_parent_MPI > 0) {
            ifmpi = 0;
        }
    }
    apptime = mysecond();
    return;
}

  __attribute__((section(".init_array"))) void *__init = libprof_init;
  __attribute__((section(".fini_array"))) void *__fini = libprof_fini;

//  void libprof_init() __attribute__((constructor));
//  void libprof_fini() __attribute__((destructor));

