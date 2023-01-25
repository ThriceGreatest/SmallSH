/***
 * CS344 Oregon State University
 * with Professor Ben Brewster
 * Assignment 3: Smallsh 
 * by Devin Martin
 * 11/9/2022
 ****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define MAXLENGTH 2048
#define SPACE " "


//Struct for holding information given on command line
struct command{
    char *args[512];    //Token string and store command arguments here
    int background_mode; //If '&' is found at end of string, we enable background mode
    char input_file[512];  //Check for '<' in string, get inputfile 
    char output_file[512]; //Check for '>' in string, get outputfile.
}command;


//GLOBAL VARIABLES

struct command command;//Initialize command struct as global variable.
char input[MAXLENGTH] = {'\0'}; //Create array for storing command line inputs.
int i = 0; //iterator
int processArray[99];//Store process PIDs
int proccesIter;//Keep track of where to place PID
int childStatus;//Keep track of Childs errror status
int errorCode; //Printable error code, converted from childstatus
int  foreground_only; //Flag for foreground only mode. If enabled, background mode must be ignored.


int prompt_command(){

    struct command new_command = {.args = {'\0'}, .background_mode = 0, .input_file = {'\0'}, .output_file = {'\0'}}; // Initialize blank command struct
    command = new_command; //Set global command struct to this blank command struct

    printf(": "); //Formatting for shell
    fflush(stdout); //Flush for safety
    fgets(input, MAXLENGTH, stdin); //Get user input and store in command

    if (input[0] == '#'){ //If first character is #... this is a comment
        //Do nothing 
        return 2;
    }
    
    else if (input[0] == '\n'){ //If the command line was blank, 
        //Do nothing
        return 2;
    }

    else if (input[0] == ' '){ //If the command line was space, 
        //Do nothing
        return 2;
    }


    else{ //It was a good command,

        for (i = 0; i < MAXLENGTH ; i++) { //Loop through entire command
            
            if (input[i] == '\n') { //Find NewLine (user pressed enter...)

                input[i] = '\0'; //Change where user pressed to be terminating character
                break;
            }
        }
    }
    return 1;
}   

int token_input(){

    //Filter/Tokenize Command into our struct for organizing data
    int count = -1; //Variable for counting how many arguments have been given to the command line
    
    char* token = strtok(input, SPACE); //Create token of first word
    char* prev_token = malloc(sizeof token); //Initilaize temp token. we will need to store the previous token to check that very last argument after exiting the loop.


    while (token != NULL){ //Loop until there are no more arguments

        strcpy(prev_token, token);//Save token as prev_token before moving on...
        count += 1;
    

        ////Handle input files..
        if(strcmp(token, "<") == 0){ //If we find '<' ...
            
            //Next token MUST be our input file
            token = strtok(NULL, SPACE); //Get next token
            if(token == NULL){break;} //Error catch
            strcpy(command.input_file, token); //Update input file to be given argument
            
            
            token = strtok(NULL, SPACE); //Get next token
            count -= 2; //keep array counter accurate
            continue;

        }

        
        ////Handle output files...
        else if(strcmp(token, ">") == 0){ //If we find '>' ...

            //Next token MUST be our output file
            token = strtok(NULL, SPACE); // Get next token
            if(token == NULL){break;} //Error catch
            strcpy(command.output_file, token);
            

            token = strtok(NULL, SPACE); //Get next token
            count -= 2; //keep array counter accurate
            continue;
        } 


        //If NOT special argument, place token as next command arugment//
        else{ 
            
            //We must preform variable expansion though...
            //use temp token and place PID where needed
            char* temp_tok = malloc(sizeof token);
            strcpy(temp_tok, token);

            for(i = 0; i < strlen(temp_tok); i++){ //Iterate through command

                if(temp_tok[i] == '$' && temp_tok[i + 1] == '$'){  //Find $$
 
                    //We are gonna cut the string and insert PID where needed,
                    char start_string[256] = {'\0'};  //temp string for front half of command...
                    char end_string[256] = {'\0'}; //temp string for back half of command
                    char pid[6] = {'\0'}; //string PID

                    strncpy(start_string, &temp_tok[0], i); //Copy start of the string

                    strcpy(end_string, &temp_tok[i + 2]); //Copy end of string

                    sprintf(pid, "%d", getpid()); //Get process ID

                    strcat(start_string, pid); //Append process ID to start of command

                    strcat(start_string, end_string); //Append end of command

                    strcpy(temp_tok, start_string); //Update temp token to hold command WITH completed variable expansion


                }

            }

            if(strcmp(token, "&") != 0){ //Make sure this isn't an '&' ...
                
                command.args[count] = temp_tok; //Place command into our argument array

            }
            
            token = strtok(NULL, SPACE); //Get next command.

        }
    }

    //Upon breaking from loop... token = NULL, prev_token = *the last argument given*
    //Check prev_token, see if it is '&'
    if(strcmp(prev_token, "&") == 0){ //If we find '&' ...
        //Enable background mode
        command.background_mode = 1;
    }

    return 1;
}


void executeCommand(){


    //**Partially From Module 5 CS344 @Oregon State University w Ben Brewster**//
    pid_t childPid = fork();
    
    if(childPid == -1){
        perror("fork() failed!");
        fflush(stdout);
        exit(1);
    }

    else if(childPid == 0){
        // Child process


        if(command.input_file[0] != '\0'){ //If there was a input file given

            int input; //int for open file
            input = open(command.input_file, O_RDONLY); //Open input file]]

            if(input == -1){//Check error
                perror(": Unable to open input file\n");
                fflush(stdout);
                exit(1);
            }

            //Else no error... continue

            int result; //temp int for errorcode
            result = dup2(input, 0); //make 0 the same thing as inputfile. (0 = STDIN). updates input to our input file.s

            if (result == -1){//Check error
                perror(": Unable to assign input file\n");
                fflush(stdout);
                exit(2);
            }

            fcntl(input, F_SETFD, FD_CLOEXEC); //Set close on execution
   
        }

        if(command.output_file[0] != '\0'){//If there was outpuf file given

            int output;//temp int for errorcode
            output = open(command.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (output == -1) {
                perror(": Unable to open output file\n");
                fflush(stdout);
                exit(1);
            }
            
            int result;
            result = dup2(output, 1);
            if (result == -1) {//Check error
                perror(": Unable to assign output file\n");
                fflush(stdout);
                exit(2);
            }
           
            fcntl(output, F_SETFD, FD_CLOEXEC); //Set close on execution

        }



        //Try to execute command
        if(execvp(command.args[0], command.args)){//If the command failed somehow... 
            
            //failed somehow:
            errorCode = 1;
            printf("%s: no such file or directory\n", command.args[0]);
            fflush(stdout);
            exit(1);
        }

        //Else it executed.
      
    } 

    else{
    // Parent

        if(command.background_mode == 1 && foreground_only == 0){
            processArray[proccesIter] = childPid; //Add child process to process array.
            proccesIter += 1;
            waitpid(childPid, &childStatus, WNOHANG); //Dont wait.
            
      //command.background_mode = 0;
            printf("background pid is %d\n", childPid);		//Output the childs PID
			fflush(stdout);
        }

        else{

            childPid = waitpid(childPid, &childStatus, 0);
            if(WIFEXITED(childStatus)){
                errorCode =  WEXITSTATUS(childStatus);
            }

            else{
                errorCode = WTERMSIG(childStatus);
            }
        }
    }
        
}


void handle_command(){
    

    if (strcmp(command.args[0], "exit") == 0){ //If first argument given was exit
        exit(0);
    }

    
    else if(strcmp(command.args[0], "cd") == 0){ //If first argument given was cd

        char *new_dir = malloc(256 * sizeof(char)); //string for holding name of new directory 
        strcpy(new_dir, getenv("HOME")); //set new directory to HOME PATH


        if(command.args[1] == NULL){ //When 'cd' is the only argument provided...

            chdir(new_dir); //go straight to home directory
    
        }

        else if(command.args[1][0] == '/'){ //When we detetect '/' as first argument, treat command as absolute path
         
           strcpy(new_dir, command.args[1]); //set new_dir to be first argument, which should be absolute path of desired directory
           chdir(new_dir); //change to directory given in first argument.

        }

        else{ //it is relative path.

            sprintf(new_dir, "%s", getcwd()); //update new_dir to be our current working direcrtory... this is where we are starting from
            strcat(new_dir, "/");
            strcat(new_dir, command.args[1]); //Append the requested directory to our path
            chdir(new_dir); //Change to new directory
        }

        free(new_dir); //Free temp string
        return;    
    }


    else if(strcmp(command.args[0], "status") == 0){ //If status was first argument

        printf("exit value %d\n", errorCode);
        fflush(stdout);
        return;

    }

    else{ //If no built in command was detected, execute unix command. 

        executeCommand();

    }

}

void catchSIGINT(int signo){
    printf("terminated by signal %d\n", signo);
	fflush(stdout);
}

void catchSIGTSTP(){
	if (foreground_only == 0){						
		foreground_only = 1;				//enable foreground mode
		printf("Entering foreground-only mode (& is now ignored)\n");
		fflush(stdout);
	}
	else{  //Exiting foreground-only mode 
		foreground_only = 0;						//reset foreground mode
	    printf("Exiting foreground-only mode\n");
        fflush(stdout);
	}
}

void rm_background_process(){

    for(i = 0; i < proccesIter; i++){ //Go through our background processes

        if(waitpid(processArray[i], &childStatus, WNOHANG) > 0){
            
            if(WIFSIGNALED(childStatus)){//check if child terminated with signal

                printf("background pid %d is done", processArray[i]); // 	
                printf(": terminated by signal %d\n", WTERMSIG(childStatus));
                fflush(stdout);

            }

            if (WIFEXITED(childStatus)){ //Check if child process terminated normally

                printf("background pid %d is done", processArray[i]); //Child PID	
                printf(": exit value %d\n", WEXITSTATUS(childStatus));
                fflush(stdout);

            }
        }
    }
}


int main(){

    //SIGINT Catch
	struct sigaction SIGINT_action = { 0 };		
    SIGINT_action.sa_handler = catchSIGINT;		
	sigfillset(&SIGINT_action.sa_mask);
	sigaction(SIGINT, &SIGINT_action, NULL);	
    
    //SIGTSTP Catch
    struct sigaction SIGTSTP_action = { 0 };	
    SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);	
    

    while(1){
        
        if(prompt_command() == 1){
            token_input();
            handle_command();
        }

        rm_background_process();
    }
    return;
}