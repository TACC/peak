  struct item* item; 
  char str[10];
  int ithread;
  bool ifrecord;

  double local_time=0.0;

ifrecord=1;
#ifdef _OPENMP
if(omp_in_parallel()) {
    ithread=omp_get_thread_num(); 
    if(ithread>0) ifrecord=0;   //only record master thread
//   printf("ifrecord=%d ithread=%d\n",ifrecord, ithread);
}
#endif

if(ifrecord)
{

  if (peakprof_debug>0) fprintf(OUTFILE,"PEAKPROF: calling %s\n",__func__);

//  memset(layer_time, 0, (MAX_LAYER*sizeof(layer_time[0]));
  for (int i=layer_count+1;i<MAX_LAYER;i++) layer_time[i]=0.0;
 // if (layer_count==0) layer_time[0]=0.0;
  layer_count++;
  
  strcpy(str,__func__);
 // if(strcmp(str,"dgemm_") != 0)
  strcpy(layer_caller[layer_count],str);
  
  //printf("calling %s --> %s\n",layer_caller[layer_count-1],str);
  item = hash_get(str); 
  if (item == NULL ) {
    item = hash_insert(str);
    strcpy(item->value.fgroup,func_group);
  }
}

  orig_f = dlsym(RTLD_NEXT, __func__);

if(ifrecord)
{
  local_time=mysecond();

// BLAS is called here
