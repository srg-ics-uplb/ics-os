/*
  Name: environment.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 04/01/04 05:04
  Description: Handles environment strings with concurrency support
    As of the moment, implementation is on a global basis only, a per process
    implementation will be implemented in the future.
*/

//finds an enviornment variable based on the key
//returns a pointer to it
env_strings *env_getstring(const char *name){
   env_strings *ptr = env_head;
   if (ptr==0) return 0;

   //wait until it is ready to access the environment
   while (env_busywait)
      ;

   //set to busy
   env_busywait = 1; 
   while (ptr!=0){
      if (strcmp(ptr->name,name)==0){
            env_busywait = 0;    
            return ptr;
      };
      ptr=ptr->next;
   };

   //done!
   env_busywait = 0;
   return 0;
};


//Return the value of an environment variable
char *env_getenv(const char *name, char *buf){

   //find the environment variable
   env_strings *ptr = env_getstring(name);

   //variable not found
   if (ptr == 0) 
      return 0;

   //check if no one is accesing the environment variable
   while (env_busywait)
      ;

   //we're in, copy the value of the variable to buf 
   env_busywait =1;
   strcpy(buf,ptr->value);

   //let go of the environment string
   env_busywait = 0;

   //return buf
   return buf;
};


// Remove a node in the environment variable
// pointer to the node is passed
int env_removenode(env_strings *node)
{
   //remove from the environment list
   if (env_head == node){
      env_head=node->next;
      node->next->prev = 0;
   }
   else{
      node->prev->next= node->next;
      if (node->next !=0)
         node->next->prev = node->prev;
   };  
    
   //deallocate memory  
   free(node->name);
   free(node->value);
   free(node);  
  
   return 0;
};


//unset an environment variable given the name
int env_unsetenv(const char *name){
   env_strings *environment;

   //validate parameter
   if (name==0) 
      return -1;

   if (strchr(name,'=')!=0) 
      return -1;

   if (strcmp(name,"")==0) 
      return -1;

   environment=env_getstring(name);

   //check if we can get in
   while (env_busywait)
      ;

   //we're in
   env_busywait = 1;

   //variable was found, remove it
   if (environment!=0){
      env_removenode(environment);
   };
  
   //give chance to others 
   env_busywait = 0;
   return 0;
};

int env_setenv(const char *name, const char *value, int replace){
   env_strings *environment;
 
   //validate parameter
   if (strchr(name,'=')!=0 || strchr(value,'=')!=0) 
      return -1;
 
   if (name==0) 
      return -1;
 
   if (value==0) 
      return -1;
 
   environment = env_getstring(name);
 
   //check if we can get in
   while (env_busywait)
      ;
 
   //we're in
   env_busywait = 1;
      
   //environment does not exist yet
   if (environment == 0){
       int namelength=strlen(name), valuelength=strlen(value);
       
       environment = (env_strings*) malloc(sizeof(env_strings));
       
       //add to the beginning, insert at head
       environment->next = env_head;
       environment->prev = 0;
       if (env_head != 0) 
         env_head->prev = environment;

       env_head = environment;
       
       //allocate spaces
       environment->name = (char*)malloc(namelength+1);
       environment->value =(char*)malloc(valuelength+1);
       
       //copy the strings
       strcpy(environment->name,name);
       strcpy(environment->value,value);
   }else if (replace) { //replace the current value
      int valuelength=strlen(value);
      
      //resize the size of the value string
      environment->value=(char*)realloc(environment->value,valuelength+1);
       
      //update the value
      strcpy(environment->value,value);
  
   };   
   //done!
   env_busywait = 0;
   return 0;
};


//Output the environment variable
void env_showenv(){
   env_strings *ptr = env_head;

   //wait until we are allowed access
   while (env_busywait)
      ;

   //we're in
   env_busywait = 1;
   while (ptr != 0){
    printf("%s=%s\n",ptr->name,ptr->value);
    ptr=ptr->next;
   };

   //give chance to others
   env_busywait = 0;
};
