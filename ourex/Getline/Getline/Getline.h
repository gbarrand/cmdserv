/* 
 * G.Barrand : copied from the pain.
 *             remove R__EXTERN and #ifndef __CINT__
 */

#ifndef OUREX_GETLINE_H
#define OUREX_GETLINE_H

/* 
 * namespace protect to avoid clash with the pain :
 */
#define Getline ourex_Getline
#define Getlinem ourex_Getlinem
#define Gl_config ourex_Gl_config
#define Gl_eof ourex_Gl_eof
#define Gl_histadd ourex_Gl_histadd
#define Gl_histinit ourex_Gl_histinit
#define Gl_in_hook ourex_Gl_in_hook
#define Gl_in_key ourex_Gl_in_key
#define Gl_out_hook ourex_Gl_out_hook
#define Gl_setwidth ourex_Gl_setwidth
#define Gl_tab_hook ourex_Gl_tab_hook
#define Gl_windowchanged ourex_Gl_windowchanged
#define Gl_erase_line ourex_Gl_erase_line /*G.Barrand*/

/*G.Barrand*/
#define Gl_is_there_stdin_input ourex_Gl_is_there_stdin_input

#ifdef __cplusplus
extern "C" {
#endif

/*typedef enum { kInit = -1, kLine1, kOneChar, kCleanUp } EGetLineMode;*/

char *Getline(const char *prompt);
char *Getlinem(int mode, const char *prompt);
void Gl_config(const char *which, int value);
void Gl_setwidth(int width);
void Gl_windowchanged();
void Gl_histinit(char *file);
void Gl_histadd(char *buf);
int  Gl_eof();

int Gl_is_there_stdin_input(int*); /*G.Barrand*/
void Gl_erase_line(); /*G.Barrand*/

extern int (*Gl_in_hook)(char *buf);
extern int (*Gl_out_hook)(char *buf);
extern int (*Gl_tab_hook)(char *buf, int prompt_width, int *cursor_loc);
extern int (*Gl_in_key)(int key);

#ifdef __cplusplus
}
#endif

#endif   /* OUREX_GETLINE_ */
