/*
  Name: script.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 20/01/04 04:51
  Description: An exteremely simple shell script interpreter
  
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
    
*/

#define SCRIPT_MAXINSTANCE 20


//removes '\n', '\r' and other characters which could mess
//with the interpreter and replaces them with a space
void script_cleanline(char *s)
{
int i;
for (i=0;s[i];i++)
    {
        if (s[i]=='\n' || s[i]=='\t' || s[i]=='\r')
          s[i]=' ';
    };
};

//loads a script file and then interprets it (In kernel mode!!!).
int script_load(const char *filename)
{
    file_PCB *handle;
    static int instance= 0;    
    int echo_command = 1;
    
    //This is to prevent infinite instances of the script interpreter
    if (instance> SCRIPT_MAXINSTANCE) return -1;
    
    handle = openfilex(filename,FILE_READ);
    if (handle==0) return -1;
    
    instance ++;
    do
        {
            char linebuffer[512],temp[512],*str,*script_command;    
            
            //obtain a line from the file
            fgets(linebuffer,512,handle);
            
            //remove control characters
            script_cleanline(linebuffer);
            
            //skip blank lines
            if (strcmp(linebuffer,"")==0) continue;
            
            /*determine if the line will be printed to the screen
             or not. Dtermined by the @ at the start or an echo off command*/
            if (linebuffer[0]!='@' && echo_command)
            printf("%s\n",linebuffer);  
              else
            if (linebuffer[0]=='@')
            linebuffer[0]=' ';     
              else
            if (linebuffer[0]==';') //a comment line? we skip this line
            continue;
            
            //filter out comments
            str = strtok(linebuffer,";");
            
            strcpy(temp,str);

            script_command = strtok(temp," ");
            
            if (strcmp(script_command,"echo")==0) 
               {
                    script_command = strtok(0," ");
                    if (strcmp(script_command,"off")==0) {echo_command = 0;continue;}
                          else
                    if (strcmp(script_command,"on")==0) {echo_command = 1;;continue;};
               };
            
            //tell the console module to execute the command
            if (console_execute(str) == -1) break;
        }
    while (!feof(handle));
    instance --;
    fclose(handle);
    return 1;
};

