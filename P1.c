#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <termios.h>


#define M_PI  3.14159265358979323846 
//M_PI was copied from math.h since the editor wasn't picking it up and was throwing an annoying error 


unsigned char * readheader(int b, int mode, unsigned char comp[5] , unsigned char MSG[70], unsigned char comp2[5] , unsigned char out[25], int msgout);
void readall(int cmd, double read);
int lehti(unsigned char hexn[], int b);
void printout(unsigned char *msg , unsigned char *read);
void printerror(unsigned char err[70]);
unsigned char * itleh(int value , int b);
void mysound(int dur, int sr, double fm, double fc, double mi, double amp);
void helphandler(char c);

int opensound();
unsigned char ** constructheader(int sr,int other_data);
void disableRawMode();
void enableRawMode();
int printmenu(char c);
void menuselect(int slc);
void dj(/*char * filename*/);
void configuresound(int audio_fd, int bps ,int chn,int sr);
void playaudio(FILE * wav_file, int audio_fd);
unsigned char ** loadpiano();
unsigned char * loadsound(char * filepath);
off_t fsize(FILE * file);
unsigned char ** loadguitar();
void exitconfirm();

int main(int argc, char **argv){

    if (argc >= 2){
        if( !(strcmp(argv[1] , "info") || strcmp(argv[1] , "rate") || strcmp(argv[1] , "channel") || strcmp(argv[1] , "volume") || strcmp(argv[1] , "generate") ) || !strcmp(argv[1]  , "--help")) 
            helphandler('s');
    }
    else  helphandler('s');

    double a;
    if(!strcmp(argv[1] , "info")){                                         
        if(argc >= 3) helphandler('i'); //helphandler exits the program, so no else needed for the next command
        readall( 0 , 0 );
    }
    else if(!strcmp(argv[1] , "rate")){
        if(argc == 3){
            if((a = atof(argv[2])) > 0)           readall( 1 , a );
            else                                  helphandler('r');
        } 
        else                                      helphandler('r');
    }   
    else if(!strcmp(argv[1] , "channel")){ 
        if(argc == 3 ){    
            if      (!strcmp(argv[2],"right"))    readall( 2 , 0 ); 
            else if  (!strcmp(argv[2],"left"))    readall( 2 , 1 );
            else                                  helphandler('c');
        }
        else                                      helphandler('c');
    }
    else if(!strcmp(argv[1] , "volume")){ 
        if(argc == 3 ){    
            if( (a = atof(argv[2])) > 0)          readall( 3 , a );
            else                                  helphandler('v');
        }
        else                                      helphandler('v');
    }
    else if(!strcmp(argv[1], "generate")){
        int opt;
        int currarg = 2;
        double inputs[6] = {3,44100,2.0,1500.0,100.0,30000.0};
        while((opt = getopt(argc, argv, ":-")) != -1){
            switch(opt){
                case '?': 
                    break;
                default:
                    if      (!strcmp(argv[currarg] , "--dur")){  if(argc > currarg)    inputs[0] = atof(argv[++currarg]); else helphandler('g');}
                    else  if(!strcmp(argv[currarg] , "--sr" )){  if(argc > currarg)    inputs[1] = atof(argv[++currarg]); else helphandler('g');}
                    else  if(!strcmp(argv[currarg] , "--fm" )){  if(argc > currarg)    inputs[2] = atof(argv[++currarg]); else helphandler('g');}
                    else  if(!strcmp(argv[currarg] , "--fc" )){  if(argc > currarg)    inputs[3] = atof(argv[++currarg]); else helphandler('g');}
                    else  if(!strcmp(argv[currarg] , "--mi" )){  if(argc > currarg)    inputs[4] = atof(argv[++currarg]); else helphandler('g');}
                    else  if(!strcmp(argv[currarg] , "--amp")){  if(argc > currarg)    inputs[5] = atof(argv[++currarg]); else helphandler('g');}
                    else  if(!strcmp(argv[currarg] , "--help")) helphandler('g');
                    else printerror((unsigned char *)"Unrecognised command");
                    currarg++;
                    break;
            }
        }
        if( currarg == argc)  mysound((int)inputs[0],(int)inputs[1],inputs[2],inputs[3],inputs[4],inputs[5]);
        else                    printerror((unsigned char *)"Unrecognised command");
    }  
    else if(!strcmp(argv[1], "dj")){
        dj(argv[2]);    
    } 
    return 0;
}

void readall(int cmd, double read){
    unsigned char outmsg[13][25] = {    "\0", 
                                        "size of file: ",
                                        "\0",
                                        "\0",
                                        "size of format chunk: ", 
                                        "WAVE type format: ", 
                                        "mono/stereo: ", 
                                        "sample rate: ",
                                        "bytes/sec: ",
                                        "block alignment: ",
                                        "bits/sample: ",
                                        "\0", 
                                        "size of data chunk: "  };
    unsigned char errors[14][70] = {    "Error! \"RIFF\" not found\0",
                                        "File size smaller than rest of header?\0", 
                                        "Error! \"WAVE\" not found\0",
                                        "Error! \"fmt \" not found\0",
                                        "Error! size of format chunk should be 16\0",
                                        "Error! WAVE type format should be 1\0",
                                        "Error! mono/stereo should be 1 or 2\0",
                                        "--blank--",
                                        "Error! bytes/second should be sample rate x block alignment\0",//8
                                        "Error! block alignment should be bits per sample / 8 x mono/stereo\0", //9
                                        "Error! bits/sample should be 8 or 16\0", //10
                                        "Error! \"data\" not found\0",
                                        "Error! insufficient data\0",
                                        "Error! bad file size (found data past the expected end of file)\0"  };
    unsigned char cmpmap[15][5] =  { {0x52,0x49,0x46,0x46,0x00}, //0  - RIFF
                                     {0x24,0x00,0x00,0x00,0x00}, //1  - SizeOfFile 
                                     {0x57,0x41,0x56,0x45,0x00}, //2  - WAVE
                                     {0x66,0x6d,0x74,0x20,0x00}, //3  - fmt
                                     {0x10,0x00,0x00,0x00,0x00}, //4  - format size
                                     {0x01,0x00,0x00,0x00,0x00}, //5  - Wave Type
                                     {0x01,0x00,0x00,0x00,0x00}, //6  - MonoStereo Down Limit (1 - 1)
                                     {0x00,0x00,0x00,0x00,0x00}, //7  - Sample Rate  (No check needed at the start)
                                     {0x00,0x00,0x00,0x00,0x00}, //8  - BytesPerSec needs rate and block align   (No check needed at the start)
                                     {0x00,0x00,0x00,0x00,0x00}, //9  - BlockAllign needs bits per sample and mono/stereo    (No check needed at the start)
                                     {0x08,0x00,0x00,0x00,0x00}, //10 - BitsPerSample (8) or 16 - First Check
                                     {0x64,0x61,0x74,0x61,0x00}, //11 - data
                                     {0x00,0x00,0x00,0x00,0x00}, //12 - Size of data (No check needed at the start)
                                     {0x02,0x00,0x00,0x00,0x00}, /*13 - MonoStereo Upper Limit (2 + 1) */
                                     {0x10,0x00,0x00,0x00,0x00}  /*14 - BitsPerSample 8 or (16) -  Second Check */};
    int bytemap[13] = {4,4,4,4,4,2,2,4,4,2,2,4,4};
    int modemap[13] = {1,3,1,1,2,2,5,0,0,0,5,1,0};
    unsigned char *header[13];

    for(int i = 0; i < 13; i++){
        if(i == 6)  header[i] = readheader(bytemap[i],modemap[i],cmpmap[i],errors[i],cmpmap[13],outmsg[i],cmd);         //  Making a sepparate check for Mono/Stereo and BitsPerSample
        else if(i == 10) {                                                                                      //  to manually input the second comp value in each one
            header[i] = readheader(bytemap[i],modemap[i],cmpmap[i],errors[i],cmpmap[14],outmsg[i],cmd);    
            if          (!(lehti(header[9],bytemap[9]) == lehti(header[10],bytemap[10]) / 8 * lehti(header[6],bytemap[6])))     printerror(errors[9]);      //Checking for BlockAllign here since it requires BitsPerSample to be validated first 
            else if     (!(lehti(header[8],bytemap[8]) == lehti(header[7],bytemap[7]) * lehti(header[9],bytemap[9])))             printerror(errors[8]);      //Checking for BytesPerSec here since it requires BlockAllign to be validated first 
        }          
        else header[i] = readheader(bytemap[i],modemap[i],cmpmap[i],errors[i],cmpmap[i],outmsg[i],cmd);                 //Normal procedure of reading each header chunk. The second cmpmap[i] isn't being used 
    }

    uint32_t sod = lehti(header[12],bytemap[12]);                                      //Sizer of data
    uint32_t sotd = lehti(header[1],bytemap[1]) - 36 - lehti(header[12],bytemap[12]);    //Size of other data
    uint16_t originalms = lehti(header[6],bytemap[6]);                                 //Original Mono/Stereo mode - Stored since it changes before it can be checked in the data output
    uint32_t Bips = lehti(header[10],bytemap[10]);

    if(cmd == 1){
        header[7] = itleh( (int)(lehti(header[7],bytemap[7]) * read) , bytemap[7] );
        header[8] = itleh( lehti(header[7],bytemap[7]) * lehti(header[9],bytemap[9]) , bytemap[8] ); 
        for(int i = 0; i < 13; i++) for(int j = 0; j < bytemap[i]; j++)                     putchar(header[i][j]);
    }
    else if(cmd == 2){
        if(originalms == 2){
            header[6]  = itleh(1,bytemap[6]);
            header[9]  = itleh( lehti(header[10],bytemap[10]) / 8 * lehti(header[6],bytemap[6]),bytemap[9]);
            header[8]  = itleh( lehti(header[7],bytemap[7]) * lehti(header[9],bytemap[9]) , bytemap[8] );
            header[12] = itleh( lehti(header[12],bytemap[12])/2,bytemap[12]);
            header[1]  = itleh(36 + (int)sotd + lehti(header[12],bytemap[12]) ,bytemap[1]);
        }
        for(int i = 0; i < 13; i++) for(int j = 0; j < bytemap[i]; j++)                     putchar(header[i][j]);
    }
    else if(cmd == 3) for(int i = 0; i < 13; i++) for(int j = 0; j < bytemap[i]; j++)       putchar(header[i][j]);
    int d = (int)read;
 
    for(unsigned int i = 0; i < sod; i++){ 
        int16_t c = getchar();
        if( c == EOF )                                                      printerror(errors[12]);
        if( cmd == 1 || ( cmd == 2 && originalms == 1) )                    putchar(c);
        else if(cmd == 2 && ( (Bips == 16 && (( i  % 2 == 1 && ((( 2 * (i+d) + 1 ) / 4 ) % 2 == 1  ) ) || ( i % 2 == 0 && ((( 2 * (i+d) - 1 ) / 4 ) % 2 == 0 )))) || ( Bips == 8 && ( i + d ) % 2 == 1 ) ) )      putchar(c);
        //Basically this if runs every 2 bytes (either the first 2, then third 3 etc or second two and fourth two) in case the BitsPerSample are 16 depending on the selected channel, and every 1 bit (first third fifth or second fourth sixth etc.) in case the BitsPerSample are 8
        //It does this by adding the current i and the i after/before it, taking the result of the integral division of them and checking whether it is odd or even.
        else if( cmd == 3 ) {
            if(Bips == 16){
                int16_t c2 = getchar(); //Reading the next byte sicne we need both for 16-bit audio volume conversion
                if(c2 == EOF) printerror(errors[12]);
                i++; // Manually moving up a place since we read another character from the list
                
                int16_t sample = (int16_t)((unsigned char)c | (c2 << 8));       // Using bitwise or and shifting to combine the two bytes into one variable
                int32_t scaled = (int32_t)(trunc(sample * read));                      //Switching to a higher size to avoid overflow. Any values exceeding the normal limits are being delt with with the ifs

                if(scaled > 32767) scaled = 32767;                             
                else if(scaled < -32768) scaled = -32768;
                putchar(scaled & 0xFF);        
                putchar((scaled >> 8) & 0xFF);
            }
            else {
                int16_t scaled = trunc(c * read);
                if(scaled > 255)    scaled = 255;
                else if(scaled < 0) scaled = 0;
                putchar(scaled); 
            }
    }

    }                                                                       //turn the fors into whiles as to avoid overflow in very large numbers
    for(unsigned int i = 0; i < sotd; i++){ //Print other data
        int16_t c = getchar();
        if(c == EOF)        printerror(errors[12]);
        if(cmd != 0)        putchar(c);
    }
    if(getchar()!=EOF)      printerror(errors[13]);
}

/**
 * @brief Reads a the appropriate ammount of
 * @param b is for the byte size that needs to be read
 * @param mode is for mode: 0 - Just returns read value , 1 - Equal Bytes , 2 - Equal (Arithmetic) , 3 - Read is greater (Arithmetic), 4 - Read is smaller (Arithmetic), 5 - Equal to one of two values
 * @param comp is the value that should be compared to the input
 * @param MSG is the message that will be printed in case of an error
 * @param comp2 is a second value for equal comparissons - only used in mode 
 * @param out is the message that will be outputed along with the value. If the inputed char array is a "\0" then no message or value is shown.
 * @param msgout turns output on/off. 0: On  Anything else: Off
 * @return read value
 */
unsigned char * readheader( int b , int mode , unsigned char comp[5] , unsigned char MSG[70] , unsigned char comp2[5] , unsigned char out[25] , int msgout ){
    unsigned char *read = (unsigned char *)calloc(b+1,sizeof(unsigned char));   //Allocate a char array with a size equal to the header chunck we are reading
    if (read == NULL)      printerror((unsigned char *)"Allocation Failed");     //Check if allocation of memory was succesfull

    for(int i = 0; i < b; i++){read[i] = (unsigned char)getchar();}

    switch(mode){
        case 0:  // Passing the value through
            if(memcmp(out,(unsigned char *)"\0",1) && !msgout)     printf("%s%d\n" , out , lehti(read,b));
            return read;
        case 1:  // Comparing the memory contents of read and comp
            if(memcmp(out,(unsigned char *)"\0",1)&& !msgout)       printf("%s%d\n" , out , lehti(read,b));
            if(memcmp(read, comp,b))                                printerror(MSG);
            return read;
        case 2: // Checks if read int value is equal to comp  
            if(memcmp(out,(unsigned char *)"\0",1)&& !msgout)       printf("%s%d\n" , out , lehti(read,b));
            if(!(lehti(read,b) == lehti(comp,b)))                   printerror(MSG);
            return read;
        case 3: // Checks if read in value is 
            if(memcmp(out,(unsigned char *)"\0",1)&& !msgout)       printf("%s%d\n" , out , lehti(read,b));
            if(lehti(read,b) <= lehti(comp,b))                      printerror(MSG); 
            return read;
        case 4:
            if(memcmp(out,(unsigned char *)"\0",1)&& !msgout)       printf("%s%d\n" , out , lehti(read,b)); 
            if(lehti(read,b) >= lehti(comp,b))                      printerror(MSG);
            return read;
        case 5:
            if(memcmp(out,(unsigned char *)"\0",1)&& !msgout)                               printf("%s%d\n" , out , lehti(read,b));
            if(!(lehti(read,b) == lehti(comp,b) || lehti(read,b) == lehti(comp2,b)))        printerror(MSG);
            return read;
        default: printerror((unsigned char *)"Achievement Unlocked: How did we get here");  //code should never reach this either
    }
    return read; // code should never reach this
}
void printerror(unsigned char err[70]){
    fprintf(stderr, "%s\n" , err);
    exit(1);
}

void mysound(int dur, int sr, double fm, double fc, double mi, double amp){
    fprintf(stderr, "%d - %d - %f - %f - %f - %f",dur,sr,fm,fc,mi,amp);
    unsigned char * header[13];
    int bytemap[13] = {4,4,4,4,4,2,2,4,4,2,2,4,4};

    header[0] = itleh(1179011410,bytemap[0]); 
    header[2] = itleh(1163280727,bytemap[2]); 
    header[3] = itleh(544501094,bytemap[3]);  
    header[4] = itleh(16,bytemap[4]);
    header[5] = itleh(1,bytemap[5]);         
    header[6] = itleh(1,bytemap[6]);       
    header[9] = itleh(2,bytemap[9]);         
    header[10] = itleh(16,bytemap[10]);     
    header[11] = itleh(1635017060,bytemap[11]); 

    header[7]  = itleh(sr,bytemap[7]); 
    header[8]  = itleh(lehti(header[7],bytemap[7]) * lehti(header[9],bytemap[9]), bytemap[8]);
    uint32_t totaldata = lehti(header[8],bytemap[8]) * dur;
    
    header[12] = itleh(totaldata, bytemap[12]);
    header[1]  = itleh(totaldata + 36, bytemap[1]);

    for(int i = 0; i < 13; i++) for(int j = 0; j < bytemap[i]; j++) putchar(header[i][j]);

    //We are calculating the total ammoung of Samples in the code, since each sample consists of 2 bytes (16 BiPS)
    uint32_t totalSamples = totaldata / 2;

    for(uint32_t i = 0; i < totalSamples; i++){ 

        double t = (double)i / (double)sr; // Finding out the time stamp we need to input into the function,
                                           // which is the sample we are currently on devided by the sample rate
        
        
        int32_t bit16 = (int32_t)trunc(amp * sin(2 * M_PI * fc * t - mi * sin(2 * M_PI * fm * t)));
        
        if(bit16 > 32767) bit16 = 32767;                             
        else if(bit16 < -32768) bit16 = -32768;  
        putchar(bit16 & 0xFF);
        putchar((bit16 >> 8) & 0xFF);
    } 
}

/**
 * @brief Turns a hex in Little Endian to the apropriate int value
 * @param hexn is the input in hex Little Endian format 
 * @return the converted number in hex form
 */
int lehti(unsigned char hexn[5], int b){
    int num = hexn[0];
    int pw = 256; //powers for the multiplication
    for(int i = 1; i < b; i++){
        num += hexn[i] * pw;
        pw *= 256;
    }
    return num;  //This function could be replaced by the most beautiful code I've seen, as shown by Aygerinos, but I'm keeping it like this, even tho it makes the code more than twice as fast
}                //num  = *(unsigned int*)(hexn+4) , truly a masterpiece. Also bitwise could be used but handling the overflow possibility is unecassery for this level of coding.
unsigned char * itleh(int value , int b){
    unsigned char *hexn = (unsigned char *)calloc(b,sizeof(unsigned char));
    for(int i = 0; i < b; i++){
        hexn[i] = value % 256;
        value /= 256;
    }
    return hexn;
}

void helphandler(char c) {
    switch(c){
        case 's':
            printf("\n=========================================Usage=========================================\n  ./soundwave <command> [argument]  <  <input .wav file>  >  <output .wav file> \n\n  Note: This program can only handle .wav files with a WAVE type format of 1\n\n  For aditional information for each command, run it with --info, or without its\n  required arguments\n");
            printf("\n===================================Available Commands==================================\n 1. info\n  > Outputs the information stored in the headerer of the wav file\n 2. rate\n  > Changes the speed of the inputed file\n 3. channel\n  > Converts a stereo audio file to monophonic of the selected channel\n 4. volume\n  > Modifies the volume of the inputed wav file according to the argument given. \n  > !WARNING! It is recommended to increase any audio by a maximum increament of 5, since any higher migh result in the audio getting corrupted !WARNING!\n 5. generate\n  > Create your own sin-patterned audio.\n  > For detailed explanation of the possible arguments, please run ./soundwave generate --help\n");
            printf("========================================================================================\n\n");
            exit(1);
        case 'i':
            printf("\n============================================info=============================================\n");
            printf("|  Usage: ./soundwave info < <input .wav file>                                               |\n");
            printf("|  Example:  ./soundwave info < input.wav                                                    |\n");
            printf("|                                                                                            |\n");
            printf("|  This command presents the following data about the .wav file to the user:                 |\n");
            printf("|                                                                                            |\n");
            printf("|   >  The total size of the file                                                            |\n");
            printf("|   >  The size of format chunk                                                              |\n");
            printf("|   >  The WAVE type format                                                                  |\n");
            printf("|   >  Whether the file is monophonic or stereophonic                                        |\n");
            printf("|   >  The sample rate of the data chunck                                                    |\n");
            printf("|   >  The speed in which the bytes need to be read in bytes/sec                             |\n");
            printf("|   >  The file's block alignment                                                            |\n");
            printf("|   >  The ammount of bits per each data sample                                              |\n");
            printf("|   >  The size of the data chunk                                                            |\n");
            printf("|                                                                                            |\n");
            printf("|  Description: In case an  error is detected in the file, an  apropriate error message is   |\n");
            printf("|  shown explaining the mistake in the wav file                                              |\n");
            printf("=============================================================================================\n\n");
            exit(1);
        case 'r':
            printf("\n============================================rate=============================================\n");    
            printf("| Usage:    ./soundwave rate <value> < <input .wav file>  >  <output .wav file>             |\n");
            printf("| Example:  ./soundwave rate 0.5 < input.wav > output.wav                                   |\n");
            printf("|                                                                                           |\n");
            printf("| Description: This  command  changes  the  rate in which the  inputed  audio file  is being|\n");
            printf("| played. The  new rate  changes  according to a decimal value  which  expreces the  desired|\n");
            printf("| *speed.                                                                                   |\n");
            printf("|                                                                                           |\n");
            printf("| An  output wav  is  required  since  otherwise  the  program  will  dump  all  the  binary|\n");
            printf("| information in the terminal in an unrecongnizable form                                    |\n");
            printf("=============================================================================================\n\n");
            exit(1);
        case 'c':
            printf("\n===========================================channel===========================================\n");
            printf("|  Usage:    ./soundwave channel <right||left> < <input .wav file>  >  <output .wav file>   |\n");
            printf("|  Example:  ./soundwave channel left < input.wav > output.wav                              |\n");
            printf("|                                                                                           |\n");
            printf("|  Description: This  command  converts  a stereophonic  audio into  a  monophonic  audio.  |\n");
            printf("|  Channel selection is up to the user. In case the inputed wav is already monophonic, the  |\n");
            printf("|  output wav will not change no matter the selected channel                                |\n");
            printf("|                                                                                           |\n");
            printf("|  An  output wav is required  since  otherwise  the  program  will  dump  all  the binary  |\n");
            printf("|  information in the terminal in an unrecongnizable form                                   |\n");
            printf("=============================================================================================\n\n");
            exit(1);
        case 'v':
            printf("\n===========================================volume============================================\n");
            printf("|  Usage:    ./soundwave volume <value> < <input .wav file>  >  <output .wav file>          |\n");
            printf("|  Example:  ./soundwave volume 2 < input.wav > output.wav                                  |\n");
            printf("|                                                                                           |\n");
            printf("|  Description: This  command  changes  the  base  volume of  the  inputed wav file. It is  |\n");
            printf("|  recommended that the volume adjustment  value is within the range of [0.1,5] to prevent  |\n");
            printf("|  any corrupted audio from forming due to the maximum and minimum allowed block value      |\n");
            printf("|                                                                                           |\n");
            printf("|  An  output wav is required  since  otherwise  the  program  will  dump  all  the binary  |\n");
            printf("|  information in the terminal in an unrecongnizable form                                   |\n");
            printf("=============================================================================================\n\n");
            exit(1);
        
        case 'g':
            printf("\n==========================================generate===========================================\n");
            printf("|  Usage:    ./soundwave generate [arguments]  >  <output .wav file>                        |\n");
            printf("|  Example:  ./soundwave generate > output.wav                                              |\n");
            printf("|       or:  ./soundwave generate --dur 5 --fm 1.2 --fc 300 > output.wav                    |\n");
            printf("|                                                                                           |\n");
            printf("|  Description: This  command  allows the user to generate their own audio files from       |\n");
            printf("|  scratch. It accepts any combinaytion of the following commands:                          |\n");
            printf("|                                                                                           |\n");
            printf("|   >  --dur <integer>                                                                      |\n");
            printf("|      Accepts the duration in seconds for the generated audio file                         |\n");
            printf("|   >  --sr  <integer>                                                                      |\n");
            printf("|      Accepts the Sample Rate for the generated audio file                                 |\n");
            printf("|   >  --fm  <decimal>                                                                      |\n");
            printf("|      Accepts the frequency modulation for the generated sin wave                          |\n");
            printf("|   >  --fc  <decimal>                                                                      |\n");
            printf("|      Accepts the carrier frequency for the generated sin wave                             |\n");
            printf("|   >  --mi  <decimal>                                                                      |\n");
            printf("|      Accepts the modulation index for the generated sin wave                              |\n");
            printf("|   >  --amp  <decimal>                                                                     |\n");
            printf("|      Accepts the amplitude for the generated sin wave                                     |\n");
            printf("|                                                                                           |\n");
            printf("|  An  output wav is required  since  otherwise  the  program  will  dump  all  the binary  |\n");
            printf("|  information in the terminal in an unrecongnizable form                                   |\n");
            printf("|                                                                                           |\n");
            printf("|  No input wav is required                                                                 |\n");
            printf("=============================================================================================\n\n");
            exit(1);
    }
}


typedef struct NoteS{
    uint8_t Code;
    struct NoteS * NextNote;
}Note;
typedef struct RecordS{
    uint8_t KD;
    uint32_t KDspacing;
    uint8_t HH[2];
    uint32_t HHspacing[2];
    Note * FirstNote;
    Note * LastNote;

}Record;

void dj(char * filename){
    FILE *outputf = fopen(filename, "r+b"); //Open the output wav for reading and writing. Reading will probably not be used
    printf("Please enter the intended Sample Rate. Press enter for default value(44100)\n Sample Rate: ");
    uint32_t sr = 44100;
    if(scanf("%u", &sr) != 1 ){
        printf("Invalid input. Defaulting to 44100");
        sr = 44100;
    }
    int audio_fd;
    configuresound(audio_fd = opensound() , 16 , 2 , sr);
    fprintf(stderr,"%ld" , fsize(outputf));

    //unsigned char buffer[4096];
    //size_t bytes_read;
    
    enableRawMode();
    
    unsigned char ** PianoNotes = NULL;
    unsigned char ** GuitarNotes = NULL;
    unsigned char * KickDrum = loadsound((char *)"DJassets/KickDrum.wav");
    unsigned char * HiHat1   = loadsound((char *)"DJassets/HiHat1.wav");
    unsigned char * HiHat2   = loadsound((char *)"DJassets/HiHat2.wav");
    fprintf(stderr,"%c - %c - %c",KickDrum[0],HiHat1[0],HiHat2[0]);
    
    Record * Memory = (Record *)calloc(5,sizeof(Record)) ;
    if (Memory == NULL) printerror((unsigned char *)"Memory allocation failed for Record Memory\n");  //Check if memory was allocated correctly
    //uint8_t saved = 0;
    char c;
    uint8_t mode = 0;
    uint8_t submode = 0;
    uint8_t finished = 0;

    fprintf(stderr,"\n\n\n\n\n\n\n\n\n");
    menuselect(1);
    
    while(!finished){

        if (read(STDIN_FILENO, &c, 1) == -1) break; //Do nothing when no input is detected 

        switch(mode){
            case 0:
                switch(c-48){
                    case 0:
                        printmenu(0);
                        exit(1);
                    case 1:
                        mode = printmenu(c);
                        break;
                    case 2: 
                        //clear beats playing
                        break;
                    case 3: case 4:
                        PianoNotes = loadpiano();
                        mode = printmenu(c);
                        break;
                    case 5:
                        //do Guide
                        break;
                    default:
                        printerror((unsigned char *) "Achievement Unlocked: How did we get here");
                }
                break;
            case 1:
                switch(c-48){
                    case 0:
                        printmenu(0);
                        exit(1);
                    case -21:
                        mode = printmenu(c);
                        free(PianoNotes);
                        break;
                    case 1: 
                        if(submode != 1)    free(GuitarNotes);
                        if(submode == 2)    PianoNotes = loadpiano();
                        submode = 1;
                        break;
                    case 2:
                        if(submode != 2)    free(PianoNotes);
                        if(submode == 1)    GuitarNotes = loadguitar();
                        submode = 2;
                    case 3:
                        if(submode == 1)    GuitarNotes = loadguitar();
                        if(submode == 2)    PianoNotes = loadpiano();
                        submode = 3;
                        break;
                    case 4:
                        //do Guide
                        break;
                    default:
                        printerror((unsigned char *) "Achievement Unlocked: How did we get here");
                }
                break;
            case 2:
                switch(c-48){
                    case 0:
                        printmenu(-1);
                        exit(1);
                    case -21:
                        mode = printmenu(c);
                        free(PianoNotes);
                        free(GuitarNotes);
                        break;
                    case 1: 
                        if(submode != 1)    free(GuitarNotes);
                        if(submode == 2)    PianoNotes = loadpiano();
                        submode = 1;
                        break;
                    case 2:
                        if(submode != 2)    free(PianoNotes);
                        if(submode == 1)    GuitarNotes = loadguitar();
                        submode = 2;
                    case 3:
                        if(submode == 1)    GuitarNotes = loadguitar();
                        if(submode == 2)    PianoNotes = loadpiano();
                        submode = 3;
                        break;
                    case 4:
                        //do Guide
                        break;
                    default:
                        printerror((unsigned char *) "Achievement Unlocked: How did we get here");
                }
                break;
            case 3:
                break;
            case 4: 
                break;
        }
        
    }

    unsigned char ** header = calloc(13,(sizeof(unsigned char *)));
    int bytemap[13] = {4,4,4,4,4,2,2,4,4,2,2,4,4};

    for(int i = 0; i <13; i++){
        header[i] = calloc(bytemap[i],(sizeof(unsigned char)));
    }

    int uod = 0;//User's other data
    //char watemark[116]= "This file was created by the DJ sub-command, using fk207's soundwave program for hw1 of introduction to programming";
    header = constructheader(sr,uod+116);
    //close(audio_fd);
    fclose(outputf);
    printf("Playback finished.\n");
    exit(0);
}

off_t fsize(FILE * file) {
    struct stat st; 

    if (fstat(fileno(file), &st) == 0)
        return st.st_size;

    return -1; 
}

unsigned char ** loadpiano(){
    unsigned char ** piano = (unsigned char **)malloc(7 * sizeof(unsigned char *));
    piano[0] = loadsound("DJassets/w1-c.wav");
    piano[1] = loadsound("DJassets/w2-d.wav");
    piano[2] = loadsound("DJassets/w3-e.wav");
    piano[3] = loadsound("DJassets/w4-f.wav");
    piano[4] = loadsound("DJassets/w5-g.wav");
    piano[5] = loadsound("DJassets/w6-a.wav");
    piano[6] = loadsound("DJassets/w7-b.wav");
    return piano;
}

unsigned char * loadsound(char * filepath){
    FILE *soundfile = fopen(filepath,"rb");
    unsigned int filesize = fsize(soundfile);
    unsigned char * buffer = (unsigned char *)malloc(filesize * sizeof(unsigned char) );
    if (buffer == NULL) {fclose(soundfile); printerror((unsigned char *)"Memory allocation failed for DJassets\n");} //Check if memory was allocated correctly
    if (fread(buffer, 1, filesize, soundfile) != filesize){fclose(soundfile); printerror((unsigned char *)"Reading error: Did not read expexted number of bytes");} //Check if file contents were read correctly

    fclose(soundfile);
    return buffer;
}

/*unsigned char ** loadguitar(){

}*/

int printmenu(char c){
    static int cm = 1;
    if(c == 0)              menuselect(0);
    else if(c == -1)        exitconfirm();
    if(cm ==1){
        if(c == '3'){       menuselect(2); cm = 2;}
        else if(c == '4'){  menuselect(3); cm = 3;}
    }
    else if(cm == 2){
        if(c == 27){        menuselect(1); cm = 1;}
    }
    else if(cm == 3){
        if(c == 27){        menuselect(1); cm = 1;}
    }
    fflush(stderr);
    return cm;
}

void exitconfirm(){
    printf("All progress and saves that have not been exported will be lost. Close program anyways? (y/n)\n");
    char c = scanf("%c",c);
    while(c != 'y' && c != 'n'){
        printf("Please enter \'y\' to exit or \'n\' to cancel.");
        scanf("%c" , c);
    }
    if(c == 'y'){
        menuselect(0);
    }
}

void menuselect(int slc){
    fprintf(stderr, "\033[2K\r");
    switch(slc){
        case 0:
            fprintf(stderr, "Closing Program");
            exit(0);
            break;
        case 1: // Standard Menu 
            fprintf(stderr, "[1] - Beat Selection    [2] - Stop Beats    [3] - Preview Mode    [4] - Record    [5] - Guide    [0] - Close Program");
            break;
        case 2: // Preview Mode
            fprintf(stderr, "[Esc] - Back To Menu    [1] - Classical Piano    [2] - Guitar    [3] - Mixed Input    [4] - Guide    [0] - Close Program");
            break;
        case 3: //Record Mode
            fprintf(stderr, "[Esc] - Stop Recording    [1] - Classical Piano    [2] - Guitar    [3] - Mixed Input    [4] - Guide    [0] - Close Program");
            break;
        case 4: //Recording Paused
            fprintf(stderr, "[E] - Export    [A] - Advanced Export    [S] - Save    [D] - Discard");
            break;
        case 5: 
            fprintf(stderr, "[1] - Kick Drum    [2] - Hi-Hat    [3] - Saves");
            break;
        default:
            printerror((unsigned char *)"Achievement Unlocked: How did we get here");
    }
}

void playaudio(FILE * wav_file, int audio_fd){
    unsigned char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), wav_file)) > 0) {
        if (write(audio_fd, buffer, bytes_read) == -1) {
            printerror((unsigned char *)"Error writing to audio device");
            break;
        }
    }
}

int opensound(){
    int audio_fd = open("/dev/dsp", O_WRONLY);
    if(audio_fd == -1){
        fprintf(stderr, "Error: Could not open /dev/dsp\n");
        fprintf(stderr, "Pleas run the command with: 'padsp ./soundwave dj <file>'\n");
        return 1;
    }
    return audio_fd;
}

void configuresound(int audio_fd, int bps ,int chn,int sr){
    int status;
    // Set bit depth (e.g., 16-bit)
    status = ioctl(audio_fd, SOUND_PCM_WRITE_BITS, &bps);
    if (status == -1) printerror((unsigned char *)"SOUND_PCM_WRITE_BITS ioctl failed");

    // Set channels (Mono/Stereo)
    status = ioctl(audio_fd, SOUND_PCM_WRITE_CHANNELS, &chn);
    if (status == -1) printerror((unsigned char *)"SOUND_PCM_WRITE_CHANNELS ioctl failed");

    // Set sample rate (e.g., 44100)
    status = ioctl(audio_fd, SOUND_PCM_WRITE_RATE, &sr);
    if (status == -1) printerror((unsigned char *)"SOUND_PCM_WRITE_RATE ioctl failed");

}

unsigned char ** constructheader(int sr,int other_data){
    unsigned char ** header = calloc(13,(sizeof(unsigned char *)));
    int bytemap[13] = {4,4,4,4,4,2,2,4,4,2,2,4,4};

    for(int i = 0; i <13; i++){
        header[i] = calloc(bytemap[i],(sizeof(unsigned char)));
    }
    header[0] = itleh(1179011410,bytemap[0]); 
    header[2] = itleh(1163280727,bytemap[2]); 
    header[3] = itleh(544501094,bytemap[3]);  
    header[4] = itleh(16,bytemap[4]);
    header[5] = itleh(1,bytemap[5]);         
    header[6] = itleh(1,bytemap[6]);       
    header[9] = itleh(2,bytemap[9]);         
    header[10] = itleh(16,bytemap[10]);     
    header[11] = itleh(1635017060,bytemap[11]); 
    header[7]  = itleh(sr,bytemap[7]); 
    header[8]  = itleh(lehti(header[7],bytemap[7]) * lehti(header[9],bytemap[9]), bytemap[8]);
    uint32_t totaldata = lehti(header[8],bytemap[8]);
    header[12] = itleh(totaldata, bytemap[12]);
    header[1]  = itleh(totaldata + 36 + other_data, bytemap[1]);


    return header;
} 

struct termios orig_termios;
void enableRawMode(){
    
    tcgetattr(STDIN_FILENO, &orig_termios);     //Saves current settings of the terminal
    atexit(disableRawMode);                     //In case the program ends unexpectedely, Raw Mode for the terminal is automatically disabled

    struct termios raw = orig_termios;          //Copying the original settings in order to modify and then push them to the input settings 
    raw.c_lflag &= ~(ICANON | ECHO);            //Disabling Canonical Mode, thus enabling Raw Mode, and disbale printing user's input in the screen

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);     //Pushing the newly configured settings 
}

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); //Re-configures the terminal back to Canonical Mode
}
