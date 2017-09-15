/** 3DGPL *************************************************\
 * ()                                                     *
 * Interpreting ascii data description, constructing a    *
 * data structure in memory, atempting to do correct      *
 * alignment for items in the structs.                    *
 *                                                        *
 * Defines:                                               *
 *  D_data                   Getting a specified export.  *
 *                                                        *
 * Internals:                                             *
 *  DI_char                  Next char of the source;     *
 *  DI_token                 Next token of the stream;    *
 *  DI_type                  Interpreting type;           *
 *  DI_var                   Interpreting var list;       *
 *  DI_length                Length of type in bytes.     *
 *                                                        *
 * (c) 1995-98 Sergei Savchenko, (savs@cs.mcgill.ca)      *
\**********************************************************/
//data.cpp

#include "LightTrack.h"           /* HW_error */
#include "data.h"                   /* self definition */
#include <stdio.h>                          /* FILE etc. */
#include <string.h>                         /* cmpstr etc. */
#include <stdlib.h>                         /* malloc etc. */

#define D_MAX_TOKEN_LENGTH 100              /* token taken from the source */
#define D_NAME_LENGTH       32              /* names gotta be this long */
#define D_BUFFER_LENGTH    256              /* chars being read */

enum D_token_type                           /* enumerates all tokens */
{
 D_EOF,                                     /* end of file */
 D_NAME,                                    /* starting from the letter */
 D_NUMBER,                                  /* starting from a number */
 D_NUMBERF,                                 /* starting from a float number */
 D_BASE_INT,                                /* base types */
 D_BASE_FLOAT,
 D_BASE_SHORT,
 D_BASE_BYTE,
 D_BASE_PTR,                                /* typeless pointer */
 D_DEREF,                                   /* dereferencing symbol */
 D_OPEN_SQ,                                 /* describes an array */
 D_CLOSE_SQ,
 D_OPEN_CURL,                               /* describes a structure */
 D_CLOSE_CURL,
 D_OPEN_RAW,                                /* describes a raw data array */
 D_CLOSE_RAW,
 D_STATE_TYPE,                              /* statements */
 D_STATE_VAR,                               /* "name" */
 D_STATE_EXPORT
};

struct D_token                              /* describes a token */
{
 char d_description[D_MAX_TOKEN_LENGTH];    /* actual text of a token */
 enum D_token_type d_type;                  /* enumeration of this token */
};

struct D_token D_tokens[]=                  /* what is in the grammer */
{
	{"int"   ,D_BASE_INT},                     /* base types */
	{"float" ,D_BASE_FLOAT},
	{"short" ,D_BASE_SHORT},
	{"byte"  ,D_BASE_BYTE},
	{"ptr"   ,D_BASE_PTR},
	{"@"     ,D_DEREF},                        /* dereferencing symbol */
	{"["     ,D_OPEN_SQ},                      /* array */
	{"]"     ,D_CLOSE_SQ},
	{"{"     ,D_OPEN_CURL},                    /* structure */
	{"}"     ,D_CLOSE_CURL},
	{"<"     ,D_OPEN_RAW},                     /* raw array */
	{">"     ,D_CLOSE_RAW},
	{"type"  ,D_STATE_TYPE},                   /* statements */
	{"var"   ,D_STATE_VAR},
	{"export",D_STATE_EXPORT},
	{""      ,(D_token_type)0}                               /* tagging the last one */
};

char D_terminators[]=" \n\r@[]{}<>\xff";    /* expression terminators */
char D_filters[]=" \n\r";                   /* characters which are ignored */

int D_current;                              /* char being read */
int D_last;                                 /* how many chars are there */
int D_line;                                 /* number of lines read */
char *D_buffer;                             /* the chars being read */
FILE *D_file;                               /* handle to the input */
char D_fname[D_NAME_LENGTH];                /* name of the current file */

struct D_int_alignment_struct { char d_char; int d_int; } D_int_check;
struct D_float_alignment_struct { char d_char; float d_float; } D_float_check;
struct D_short_alignment_struct { char d_char; short d_short; } D_short_check;
struct D_ptr_alignment_struct { char d_char; char *d_ptr; } D_ptr_check;
int D_int_alignment;                        /* alignment type for int */
int D_float_alignment;                      /* alignment type for int */
int D_short_alignment;                      /* alignment type for int */
int D_ptr_alignment;                        /* gotta be same as int but... */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Stream of chars from the script file.                 *
 *                                                       *
 * RETURNS: Next character from the script stream.       *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

char DI_char(void)
{
 if(D_current<D_last)                       /* as long as have something */
 {
  return(D_buffer[D_current++]);            /* current char */
 }
 else
 {
  if((D_last=fread(D_buffer,sizeof(char),D_BUFFER_LENGTH,D_file))==0)
   return('\xff');                          /* end of the source */
  D_current=0;                              /* starting from the first one */
  return(D_buffer[D_current++]);
 }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Tokenizer. token is defined as a text between         *
 * a filter ( ignored ) and a terminator. token might    *
 * be a terminator.                                      *
 *                                                       *
 * RETURNS: Type of the current token.                   *
 * --------                                              *
 * SETS: D_token_text,D_token_number for some kinds of   *
 * ----- tokens, D_line would have number of \n read.    *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

char D_token_text[D_MAX_TOKEN_LENGTH];      /* set after DI_token */
int D_token_number;
float D_token_numberf;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

enum D_token_type DI_token(void)
{
 int i,lng=0;                               /* empty yet */
 char c;

 for(;;)                                    /* ignore preceding filters */
 {
  c=DI_char();
  if(c=='\xff')                             /* end of input strem */
  {
   D_token_text[lng]=0;                     /* no text associated here */
   return(D_EOF);                           /* end of the source */
  }
  for(i=0;D_filters[i]!=0;i++)
   if(c==D_filters[i]) break;               /* yes, filter current symbol */
  if(D_filters[i]==0)
  {
   D_current--;                             /* process further */
   break;                                   /* nope, none of filters */
  }
  if(D_filters[i]=='\n') D_line++;          /* counter of lines read */
 }

 for(;;)                                    /* copy until the terminator */
 {
  c=DI_char();
  for(i=0;D_terminators[i]!=0;i++)
   if(c==D_terminators[i]) break;           /* yes, terminate on this one */
  if(D_terminators[i]==0)                   /* not a terminator */
  {
   D_token_text[lng++]=c;                   /* char in the token */
  }
  else                                      /* yes a terminator */
  {
   D_token_text[lng]=0;                     /* end of line */
   D_current--;                             /* process a terminator further */
   break;                                   /* finished composing a token */
  }
 }

 if(lng==0)                                 /* checking tokens/terminators */
 {
  for(i=0;strlen(D_tokens[i].d_description)!=0;i++)
   if(strncmp(D_tokens[i].d_description,&c,1)==0)
   {
    D_current++;                            /* don't process on the next step */
    return(D_tokens[i].d_type);
   }
 }
 for(i=0;strlen(D_tokens[i].d_description)!=0;i++)
  if(strcmp(D_tokens[i].d_description,D_token_text)==0)
   return(D_tokens[i].d_type);              /* command token */

 if(strchr(D_token_text,'.')!=NULL)
 {
  if(sscanf(D_token_text,"%f",&D_token_numberf)==1)
   return(D_NUMBERF);                       /* is floating point */
 }

 if((lng>2)&&(D_token_text[0]=='0')&&
             (D_token_text[1]=='x'))        /* 0x0 hexadecimal */
 {
  if(sscanf(D_token_text+2,"%x",&D_token_number)==1)
   return(D_NUMBER);                        /* is hexadecimal */
 }
 if(sscanf(D_token_text,"%d",&D_token_number)==1)
  return(D_NUMBER);                         /* is decimal */

 return(D_NAME);                            /* can be only a name */
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * The processing of the token stream from the moment    *
 * the function is called should relate to type          *
 * description as follows:                               *
 *                                                       *
 * type = <int> | <float> | <short> | <byte> | <ptr> |   *
 *      | <[>number<]>type | <{> type {type} <}>         *
 *                                                       *
 * RETURNS: Number of the type just created.             *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct D_type_name                          /* relates type name and number */
{
 char d_name[D_NAME_LENGTH];                /* type name */
 int d_type;                                /* type number */
};

struct D_type                               /* describes a type */
{
 int d_array;                               /* number of elements or -1 */
 int d_next;                                /* next item or -1 */
 int d_current;                             /* this type */
};

struct D_var_name                           /* description of a variable */
{
 char d_name[D_NAME_LENGTH];                /* name of a variable */
 char *d_data;                              /* allocated data */
 int d_type;                                /* type of a variable */
};

#define D_SINGULAR    0                     /* not an array type */
#define D_NONE       -1                     /* kinda internal NULL */
#define D_TYPE_INT   -2                     /* base types: integer */
#define D_TYPE_FLOAT -3
#define D_TYPE_SHORT -4
#define D_TYPE_BYTE  -5                     /* byte */
#define D_TYPE_PTR   -6                     /* and pointer */

#define D_MAX_TYPES      512                /* sizes for tables */
#define D_MAX_VAR_NAMES  512
#define D_MAX_TYPE_NAMES 512

struct D_type_name *D_type_names;           /* relates names to numbers */
int D_no_type_names;                        /* number of names */
struct D_type *D_types;                     /* all types */
int D_no_types;                             /* number of all types */
struct D_var_name *D_var_names;             /* all variable names */
int D_no_var_names;                         /* number of variables */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int DI_type(void)
{
 int prev,orig=0,tp,i;

 switch(DI_token())
 {
  case D_NAME:      for(i=0;i<D_no_type_names;i++)
                     if(strcmp(D_token_text,D_type_names[i].d_name)==0)
                      return(D_type_names[i].d_type);
                    HW_error("(%s:%d) Unknown type name: %s\n",D_fname,D_line,D_token_text);
                    break;

  case D_BASE_INT:  return(D_TYPE_INT);     /* base type int */

  case D_BASE_FLOAT: return(D_TYPE_FLOAT);  /* base type float */

  case D_BASE_SHORT:return(D_TYPE_SHORT);   /* base type short */

  case D_BASE_BYTE: return(D_TYPE_BYTE);    /* byte type byte */

  case D_BASE_PTR:  return(D_TYPE_PTR);     /* base type typeless ptr */

  case D_OPEN_SQ:   if(DI_token()!=D_NUMBER)/* array [N] */
                     HW_error("(%s:%d) Number expected.\n",D_fname,D_line);

                    D_types[D_no_types].d_array=D_token_number;
                    D_types[tp=D_no_types++].d_next=-1;

                    if(D_no_types>=D_MAX_TYPES)
                     HW_error("(Data) No space in type list.\n");

                    if(DI_token()!=D_CLOSE_SQ)
                     HW_error("(%s:%d) Closing bracket expected.\n",
                              D_fname,D_line
                             );

                    D_types[tp].d_current=DI_type();
                    return(tp);             /* was an array type */

  case D_OPEN_CURL: prev=D_NONE;            /* no first in the chain yet */
                    while((tp=DI_type())!=D_NONE)
                    {
                     D_types[D_no_types].d_array=D_SINGULAR;
                     D_types[D_no_types].d_current=tp;
                     tp=D_no_types++;       /* just created type is */

                     if(D_no_types>=D_MAX_TYPES)
                      HW_error("(Data) No space in type list.\n");

                     if(prev!=D_NONE)       /* extending the chain */
                      D_types[prev].d_next=tp;
                     else
                      orig=tp;              /* head of the chain */
                     prev=tp;               /* to allow extention further */
                    }
                    D_types[prev].d_next=D_NONE;
                    return(orig);           /* head of the chain */

  case D_CLOSE_CURL:return(D_NONE);         /* no more types in the list */

  default:          HW_error("(%s:%d) Type defenition expected.\n",
                             D_fname,D_line
                            );
 }

 return(D_NONE);                            /* would never happen, actually */
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * The processing of the token stream from the moment    *
 * the function is called should correspond to passed    *
 * type and relate to variable defenition as follows:    *
 *                                                       *
 * var = number | reference | @reference | file_name |   *
 *     | <[> var {var} <]> | <{> var {var} <}>           *
 *                                                       *
 * RETURNS: Number of bytes filled by the definition.    *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int DI_var(int type,char* space)
{
 enum D_token_type tp;
 char *orig_space=space;                    /* where the start was */
 int i;

 if(type==D_TYPE_BYTE)                      /* base type byte */
 {
  switch(DI_token())                        /* number or @name */
  {
   case D_NUMBER: *(char*)space=(char)D_token_number;
                  return(sizeof(char));

   case D_DEREF:  DI_token();               /* getting name */
                  for(i=0;i<D_no_var_names;i++)
                   if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                  if(i==D_no_var_names)
                   HW_error("(%s:%d) Unknown name %s.\n",
                            D_fname,D_line,D_token_text
                           );

                  if(D_var_names[i].d_type!=D_TYPE_BYTE)
                   HW_error("(%s:%d) %s Expected to be a constant.\n",
                            D_fname,D_line,D_token_text
                           );

                  *(char*)space=*(char*)D_var_names[i].d_data;
                  return(sizeof(char));

   default:       HW_error("(%s:%d) Constant expected.\n",D_fname,D_line);
  }
 }

 if(type==D_TYPE_SHORT)                     /* base type short */
 {
  if(((long)space)%D_short_alignment!=0)
   space+=D_short_alignment-((long)space)%D_short_alignment;

  switch(DI_token())                        /* number of @name */
  {
   case D_NUMBER: *(short*)space=(short)D_token_number;
                  space+=sizeof(short);
                  return(space-orig_space); /* pointer difference */

   case D_DEREF:  DI_token();               /* getting name */
                  for(i=0;i<D_no_var_names;i++)
                   if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                  if(i==D_no_var_names)
                   HW_error("(%s:%d) Unknown name %s.\n",
                            D_fname,D_line,D_token_text
                           );

                  if(D_var_names[i].d_type!=D_TYPE_SHORT)
                   HW_error("(%s:%d) %s Expected to be a constant.\n",
                            D_fname,D_line,D_token_text
                           );

                  *(short*)space=*(short*)D_var_names[i].d_data;
                  space+=sizeof(short);
                  return(space-orig_space); /* pointer difference */

   default:       HW_error("(%s:%d) Constant expected.\n",D_fname,D_line);
  }
 }

 if(type==D_TYPE_INT)                       /* base type int */
 {
  if(((long)space)%D_int_alignment!=0)
   space+=D_int_alignment-((long)space)%D_int_alignment;

  switch(DI_token())                        /* number of @name */
  {
   case D_NUMBER: *(int*)space=D_token_number;
                  space+=sizeof(int);
                  return(space-orig_space); /* pointer difference */

   case D_DEREF:  DI_token();               /* getting name */
                  for(i=0;i<D_no_var_names;i++)
                   if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                  if(i==D_no_var_names)
                   HW_error("(%s:%d) Unknown name %s.\n",
                            D_fname,D_line,D_token_text
                           );

                  if(D_var_names[i].d_type!=D_TYPE_INT)
                   HW_error("(%s:%d) %s Expected to be a constant.\n",
                            D_fname,D_line,D_token_text
                           );

                  *(int*)space=*(int*)D_var_names[i].d_data;
                  space+=sizeof(int);
                  return(space-orig_space); /* pointer difference */

   default:       HW_error("(%s:%d) Constant expected.\n",D_fname,D_line);
  }
 }

 if(type==D_TYPE_FLOAT)                     /* base type float */
 {
  if(((long)space)%D_float_alignment!=0)
   space+=D_float_alignment-((long)space)%D_float_alignment;

  switch(DI_token())                        /* number of @name */
  {
   case D_NUMBERF:*(float*)space=D_token_numberf;
                  space+=sizeof(float);
                  return(space-orig_space); /* pointer difference */

   case D_NUMBER: *(float*)space=(float)D_token_number;
                  space+=sizeof(float);
                  return(space-orig_space); /* pointer difference */

   case D_DEREF:  DI_token();               /* getting name */
                  for(i=0;i<D_no_var_names;i++)
                   if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                  if(i==D_no_var_names)
                   HW_error("(%s:%d) Unknown name %s.\n",
                            D_fname,D_line,D_token_text
                           );

                  if(D_var_names[i].d_type!=D_TYPE_FLOAT)
                   HW_error("(%s:%d) %s Expected to be a constant.\n",
                            D_fname,D_line,D_token_text
                           );

                  *(float*)space=*(float*)D_var_names[i].d_data;
                  space+=sizeof(float);
                  return(space-orig_space); /* pointer difference */

   default:       HW_error("(%s:%d) Constant expected.\n",D_fname,D_line);
  }
 }

 if(type==D_TYPE_PTR)                       /* base type pointer */
 {
  if(((long)space)%D_ptr_alignment!=0)
   space+=D_ptr_alignment-((long)space)%D_ptr_alignment;

  switch(DI_token())                        /* 0 or @name or name */
  {
   case D_NUMBER: if(D_token_number!=0)
                   HW_error("(%s:%d) Only \"0\" allowed as a pointer.\n",
                            D_fname,D_line
                           );
                  *(char**)space=NULL;      /* 0 == NULL */
                  space+=sizeof(char*);
                  return(space-orig_space); /* pointer difference */

   case D_NAME:   for(i=0;i<D_no_var_names;i++)
                   if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                  if(i==D_no_var_names)     /* not a name, a file? */
                  {
                   int DD_current=D_current;/* stack of globals for */
                   int DD_last=D_last;      /* recursive call to D_data */
                   int DD_line=D_line;
                   char *DD_buffer=D_buffer;
                   FILE *DD_file=D_file;
                   struct D_type_name *DD_type_names=D_type_names;
                   int DD_no_type_names=D_no_type_names;
                   struct D_type *DD_types=D_types;
                   int DD_no_types=D_no_types;
                   struct D_var_name *DD_var_names=D_var_names;
                   int DD_no_var_names=D_no_var_names;
                   char DD_fname[D_NAME_LENGTH];
                   strncpy(DD_fname,D_fname,D_NAME_LENGTH);

                   if(strcmp(DD_fname,D_token_text)==0)
                    HW_error("(%s:%d) Can't recurse files, think about it.\n",
                             D_fname,D_line
                            );

                   *(char**)space=(char *)D_data(D_token_text);

                   strncpy(D_fname,DD_fname,D_NAME_LENGTH);
                   D_no_var_names=DD_no_var_names;
                   D_var_names=DD_var_names;
                   D_no_types=DD_no_types;
                   D_types=DD_types;
                   D_no_type_names=DD_no_type_names;
                   D_type_names=DD_type_names;
                   D_file=DD_file;
                   D_buffer=DD_buffer;
                   D_line=DD_line;
                   D_last=DD_last;
                   D_current=DD_current;    /* restoring globals from stack */
                  }
                  else                      /* a name */
                  {
                   *(char**)space=D_var_names[i].d_data;
                  }
                  space+=sizeof(char*);
                  return(space-orig_space); /* pointer difference */

   case D_DEREF:  DI_token();               /* getting name */
                  for(i=0;i<D_no_var_names;i++)
                   if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                  if(i==D_no_var_names)
                   HW_error("(%s:%d) Unknown name %s.\n",
                            D_fname,D_line,D_token_text
                           );

                  if(D_var_names[i].d_type!=D_TYPE_PTR)
                   HW_error("(%s:%d) %s Expected to be a constant pointer.\n",
                            D_fname,D_line,D_token_text
                           );

                  *(char**)space=*(char**)D_var_names[i].d_data;
                  space+=sizeof(char*);
                  return(space-orig_space); /* pointer difference */

   default:       HW_error("(%s:%d) Name or \"0\" expected.\n",D_fname,D_line);
  }
 }

 if(D_types[type].d_array!=D_SINGULAR)      /* type is an array */
 {
  tp=DI_token();
  if(tp==D_OPEN_RAW)                        /* raw byte array */
  {
   if(D_types[type].d_current!=D_TYPE_BYTE) /* raw array starts with "<" */
    HW_error("(%s:%d) Raw array is allowed only for bytes.\n",D_fname,D_line);

   for(i=0;i<D_types[type].d_array;i++,space++)
    *space=DI_char();

   if(DI_token()!=D_CLOSE_RAW)              /* raw array ends with ">" */
    HW_error("(%s:%d) Closing bracket expected.\n",D_fname,D_line);

   return(space-orig_space);                /* pointer difference */
  }
  else
  {
   if(tp!=D_OPEN_SQ)                        /* array starts with "[" */
    HW_error("(%s:%d) Opening bracket expected.\n",D_fname,D_line);

   for(i=0;i<D_types[type].d_array;i++)     /* processing elements */
    space+=DI_var(D_types[type].d_current,space);

   if(DI_token()!=D_CLOSE_SQ)               /* array ends with "]" */
    HW_error("(%s:%d) Closing bracket expected.\n",D_fname,D_line);

   return(space-orig_space);                /* pointer difference */
  }
 }

 if(D_types[type].d_next!=D_NONE)           /* a structure? */
 {
  if(DI_token()!=D_OPEN_CURL)               /* struct starts with "{" */
   HW_error("(%s:%d) Opening bracket expected.\n",D_fname,D_line);

  for(;type!=D_NONE;type=D_types[type].d_next)
   space+=DI_var(D_types[type].d_current,space);

  if(DI_token()!=D_CLOSE_CURL)              /* struct ends with "}" */
   HW_error("(%s:%d) Closing bracket expected.\n",D_fname,D_line);

  return(space-orig_space);                 /* pointer difference */
 }
 return(space-orig_space);                  /* pointer difference */
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Size of a type defined in types table.                *
 *                                                       *
 * RETURNS: Length in bytes of the requested type.       *
 * --------                                              *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int DI_length(int type,int current_length)
{
 int alignment,lng=0;

 if(type==D_TYPE_BYTE) return(sizeof(char));/* base types */
 if(type==D_TYPE_SHORT)
 {
  if(current_length%D_short_alignment==0) alignment=0;
  else alignment=D_short_alignment-current_length%D_short_alignment;
  return(sizeof(short)+alignment);
 }
 if(type==D_TYPE_INT)
 {
  if(current_length%D_int_alignment==0) alignment=0;
  else alignment=D_int_alignment-current_length%D_int_alignment;
  return(sizeof(int)+alignment);
 }
 if(type==D_TYPE_FLOAT)
 {
  if(current_length%D_float_alignment==0) alignment=0;
  else alignment=D_float_alignment-current_length%D_float_alignment;
  return(sizeof(float)+alignment);
 }
 if(type==D_TYPE_PTR)
 {
  if(current_length%D_ptr_alignment==0) alignment=0;
  else alignment=D_ptr_alignment-current_length%D_ptr_alignment;
  return(sizeof(char*)+alignment);
 }

 if(D_types[type].d_array!=D_SINGULAR)      /* type is an array */
 {                                          /* first item might be aligned */
  alignment=DI_length(D_types[type].d_current,current_length);
  return((D_types[type].d_array-1)*DI_length(D_types[type].d_current,0)+
         alignment
        );
 }

 if(D_types[type].d_next!=D_NONE)           /* a structure? */
 {
  for(;type!=D_NONE;type=D_types[type].d_next)
   lng+=DI_length(D_types[type].d_current,lng);
 }

 return(lng);
}

/*********************************************************\
 * Processing specified input file remembering type info *
 * in <type> statements, constructing variables on <var> *
 * statements processing stops on first export statement.*
 *                                                       *
 * source = <type> name type |                           *
 *        | <var> type name var |                        *
 *        | <export> name                                *
 *                                                       *
 * RETURNS: Pointer to data specified in first export.   *
 * --------                                              *
\*********************************************************/

void* D_data(char *name)
{
 char *data=NULL;                           /* pointer to be returned */
 int tp,i,quit=0;                           /* flag when to quit */
 long lng;

 D_int_alignment=((char*)&D_int_check.d_int)-((char*)&D_int_check.d_char);
 D_float_alignment=((char*)&D_float_check.d_float)-((char*)&D_float_check.d_char);
 D_short_alignment=((char*)&D_short_check.d_short)-((char*)&D_short_check.d_char);
 D_ptr_alignment=((char*)&D_ptr_check.d_ptr)-((char*)&D_ptr_check.d_char);

 strncpy(D_fname,name,D_NAME_LENGTH);       /* for error messages */
 if((D_file=fopen(name,"rb"))==NULL)        /* where to take data from */
    HW_error("(Data) Can't open %s.\n",D_fname);
 D_current=D_last=0;                        /* the buffer is empty */
 D_line=1;                                  /* initally at first line */
 D_buffer=(char*)malloc(D_BUFFER_LENGTH*sizeof(char));
 if(D_buffer==NULL) HW_error("(Data) Not enough memory.\n");

 D_type_names=(struct D_type_name*)malloc(D_MAX_TYPE_NAMES*
               sizeof(struct D_type_name));
 D_types=(struct D_type*)malloc(D_MAX_TYPES*sizeof(struct D_type));
 D_var_names=(struct D_var_name*)malloc(D_MAX_VAR_NAMES*
              sizeof(struct D_var_name));
 if((D_type_names==NULL)||(D_types==NULL)||(D_var_names==NULL))
  HW_error("(Data) Not enough memory.\n");

 D_no_type_names=D_no_types=D_no_var_names=0;/* all tables are empty */

 do
 {
  switch(DI_token())
  {
   case D_STATE_TYPE:  if(DI_token()!=D_NAME)
                        HW_error("(%s:%d) Name expected.\n",D_fname,D_line);

                       for(i=0;i<D_no_type_names;i++)
                        if(strcmp(D_token_text,D_type_names[i].d_name)==0)
                         HW_error("(%s:%d) Duplicate type name %s.\n",
                                  D_fname,D_line,D_token_text
                                 );

                       strncpy(D_type_names[D_no_type_names].d_name,
                               D_token_text,D_NAME_LENGTH
                              );
                       D_type_names[D_no_type_names++].d_type=DI_type();

                       if(D_no_type_names>=D_MAX_TYPE_NAMES)
                        HW_error("(Data) No space in type table.\n");

                       break;

   case D_STATE_VAR:   D_var_names[D_no_var_names].d_data=(char*)
                       malloc(lng=sizeof(char)*DI_length(tp=DI_type(),0));
                       if(D_var_names[D_no_var_names].d_data==NULL)
                        HW_error("(Data) Not enough memory %ld.\n",lng);

                       D_var_names[D_no_var_names].d_type=tp;

                       if(DI_token()!=D_NAME)
                        HW_error("(%s:%d) Name expected.\n",D_fname,D_line);

                       for(i=0;i<D_no_var_names;i++)
                        if(strcmp(D_token_text,D_var_names[i].d_name)==0)
                         HW_error("(%s:%d) Duplicate variable name %s.\n",
                                  D_fname,D_line,D_token_text
                                 );

                       strncpy(D_var_names[D_no_var_names].d_name,
                               D_token_text,D_NAME_LENGTH
                              );
                       DI_var(tp,D_var_names[D_no_var_names++].d_data);

                       if(D_no_var_names>=D_MAX_VAR_NAMES)
                        HW_error("(Data) No space in var table.\n");

                       break;

   case D_STATE_EXPORT:if(DI_token()!=D_NAME)
                        HW_error("(%s:%d) Name expected.\n",D_fname,D_line);

                       for(i=0;i<D_no_var_names;i++)
                        if(strcmp(D_token_text,D_var_names[i].d_name)==0) break;

                       if(i==D_no_var_names)
                        HW_error("(%s:%d) Unknown variable name %s.\n",
                                 D_fname,D_line,D_token_text
                                );

                       data=D_var_names[i].d_data;
                       quit=1;              /* quit processing */
                       break;
   default:            HW_error("(%s:%d) No such statement %s.\n",
                                D_fname,D_line,D_token_text
                               );
  }
 } while(!quit);

 fclose(D_file);
 free(D_buffer);                            /* getting rid of all tables */
 free(D_type_names);
 free(D_types);
 free(D_var_names);

 return(data);                              /* constructed structure */
}

/**********************************************************/
