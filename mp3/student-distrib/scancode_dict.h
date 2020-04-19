/*translate from https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html*/
#ifndef _SCANCODE_DICT_H
#define _SCANCODE_DICT_H

#define DICT_SIZE 0x3A

/*This table is indexed by the scancode from the keyboard
 *each entry consisits of the lowercase and uppercase of the 
 *character*/
unsigned char scancode_dict[DICT_SIZE][2] = 
{
       {   0,0   } ,             //unsupported by cp1
       {   0,0   } ,
       { '1','!' } ,
       { '2','@' } ,
       { '3','#' } ,
       { '4','$' } ,
       { '5','%' } ,
       { '6','^' } ,
       { '7','&' } ,
       { '8','*' } ,
       { '9','(' } ,
       { '0',')' } ,
       { '-','_' } ,
       { '=','+' } ,
       {   8,8   } ,        //Backspace
       {   0,0   } ,        //unsupported by cp1
       { 'q','Q' } ,
       { 'w','W' } ,
       { 'e','E' } ,
       { 'r','R' } ,
       { 't','T' } ,
       { 'y','Y' } ,
       { 'u','U' } ,
       { 'i','I' } ,
       { 'o','O' } ,
       { 'p','P' } ,
       { '[','{' } ,
       { ']','}' } ,
       {'\n','\n'} ,          //ENTER, should give back a newline charcater
       {   0,0   } ,        //unsupported by cp1
       { 'a','A' } ,
       { 's','S' } ,
       { 'd','D' } ,
       { 'f','F' } ,
       { 'g','G' } ,
       { 'h','H' } ,
       { 'j','J' } ,
       { 'k','K' } ,
       { 'l','L' } ,
       { ';',':' } ,
       {  39, 34 } ,          //comma
       { '`','~' } ,
       {   0,0   } ,        //unsupported by cp1
       { '\\','|'} ,
       { 'z','Z' } ,
       { 'x','X' } ,
       { 'c','C' } ,
       { 'v','V' } ,
       { 'b','B' } ,
       { 'n','N' } ,
       { 'm','M' } ,
       { ',','<' } ,
       { '.','>' } ,
       { '/','?' } ,
       {   0,0   } ,        //unusupported by cp1
       {   0,0   } ,        //unsupported by cp1
       {   0,0   } ,
       { ' ',' ' } ,
       
};


#endif

