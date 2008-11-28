/*
  Name: environment.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 04/01/04 05:04
  Description: Handles environment strings with concurrency support
    As of the moment, implementation is on a global basis only, a per process
    implementation will be implemented in the future.
*/

//finds an enviornment string
env_strings *env_getstring(const char *name)
{
env_strings *ptr = env_head;
if (ptr==0) return 0;
//wait until it is ready to access the environments
while (env_busywait);
//set to busy
env_busywait = 1; 
while (ptr!=0)
{
    if (strcmp(ptr->name,name)==0)
        {
            env_busywait = 0;    
            return ptr;
        };
        
ptr=ptr->next;
};

//done!
env_busywait = 0;
return 0;
};

char *env_getenv(const char *name,char *buf)
{
    env_strings *ptr = env_getstring(name);
    if (ptr == 0) return 0;
    while (env_busywait);
    env_busywait =1;
    strcpy(buf,ptr->value);
    env_busywait = 0;
    return buf;
};

int env_removenode(env_strings *node)
{
  //remove from the environment list
  if (env_head == node)
  {
    env_head=node->next;
    node->next->prev = 0;
  }
      else
  {
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

int env_unsetenv(const char *name)
{
env_strings *environment;

//validate parameter
if (name==0) return -1;
if (strchr(name,'=')!=0) return -1;
if (strcmp(name,"")==0) return -1;
environment=env_getstring(name);

while (env_busywait);
env_busywait = 1;

if (environment!=0) 
{
    env_removenode(environment);
};
env_busywait = 0;
return 0;
};

int env_setenv(const char *name, const char *value, int replace)
{
 env_strings *environment;
 //validate parameter
 if (strchr(name,'=')!=0 || strchr(value,'=')!=0) return -1;
 if (name==0) return -1;
 if (value==0) return -1;
 environment = env_getstring(name);
 
 while (env_busywait);
 env_busywait = 1;
      
 if (environment == 0) 
 {
       int namelength=strlen(name), valuelength=strlen(value);
       
       environment = (env_strings*) malloc(sizeof(env_strings));
       
       //add to the beginning
       environment->next = env_head;
       environment->prev = 0;
       if (env_head!=0) env_head->prev = environment;
       env_head = environment;
       
       //allocate spaces
       environment->name = (char*)malloc(namelength+1);
       environment->value =(char*)malloc(valuelength+1);
       
       //copy the strings
       strcpy(environment->name,name);
       strcpy(environment->value,value);
 }
    else if (replace)
 {
       int valuelength=strlen(value);
       //resize the size of the value string
       environment->value=(char*)realloc(environment->value,valuelength+1);
       
       //update the value
       strcpy(environment->value,value);
  
 };   
 //done!
 env_busywait = 0;
 return 1;
};

void env_showenv()
{
env_strings *ptr= env_head;
while (env_busywait);
env_busywait = 1;
while (ptr!=0)
{
    printf("%s=%s\n",ptr->name,ptr->value);
    ptr=ptr->next;
};
env_busywait = 0;
};
