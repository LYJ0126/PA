#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  printf("\n");
  if(cmd[0] == '\n') return;
  if (strcmp(cmd, "help") == 0) {
    sh_printf("Built-in commands:\n");
    sh_printf("  help: show this message\n");
    sh_printf("  hello: say hello to the world\n");
    sh_printf("  exit: exit the shell\n");
  } else if (strcmp(cmd, "hello") == 0) {
    sh_printf("Hello, world!\n");
  } else if (strcmp(cmd, "exit") == 0) {
    exit(0);
  }
  //获取文件名
  char *args = strtok((char *)cmd, "\n");
  if(args == NULL) return;
  if(execvp(args, NULL) == -1) {
    sh_printf("文件为找到\n");
  }
  return;
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();
  setenv("PATH", "/bin", 0);
  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
