#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Widget Widget;

typedef struct {
    void (*render)(Widget *self);
    void (*on_event)(Widget *self, int code);
} VTable;

struct Widget {
    const VTable *vtbl; 
    int id;
    int closed;
    char label[24];
};

#define MAX_WIDGETS 8
typedef struct {
    Widget *items[MAX_WIDGETS];
    int count;
} Screen;

/* ── 위젯 종류별 동작 ─────────────────────────────────────────── */
static void button_render(Widget *self) {
    printf("  [Button #%d] \"%s\"\n", self->id, self->label);
}
static void label_render(Widget *self) {
    printf("  Label #%d: %s\n", self->id, self->label);
}
static void dialog_render(Widget *self) {
    printf("  <<Dialog #%d>> %s\n", self->id, self->label);
}

static void widget_noop_event(Widget *self, int code) { (void)self; (void)code; }

/* 다이얼로그는 이벤트 코드 1(닫기)을 받으면 스스로 정리(파괴)된다 */
static void dialog_on_event(Widget *self, int code);

static const VTable BUTTON_VT = { button_render, widget_noop_event };
static const VTable LABEL_VT  = { label_render,  widget_noop_event };
static const VTable DIALOG_VT = { dialog_render, dialog_on_event  };

static Widget *widget_new(const VTable *vt, int id, const char *label) {

    Widget *w = malloc(sizeof *w);
    if (!w) { perror("malloc"); exit(1); }
    w->vtbl = vt;
    w->id = id;
    w->closed = 0;
    strncpy(w->label, label, sizeof(w->label) - 1);
    w->label[sizeof(w->label) - 1] = '\0';
    return w;
}

static void widget_destroy(Widget *w) {
    free(w);
}

/* ── Screen ──────────────────────────────────────────────────── */
static void screen_add(Screen *s, Widget *w) {
    if (s->count < MAX_WIDGETS) s->items[s->count++] = w;
}

static void screen_dispatch(Screen *s, int code) {
    for (int i = 0; i < s->count; i++) {
        Widget *w = s->items[i];
        w->vtbl->on_event(w, code);
    }
}

static void screen_render(Screen *s) {
    for (int i = 0; i < s->count; i++) {
        Widget *w = s->items[i];
        w->vtbl->render(w);       // 여기서 멈춤
    }
}

static void dialog_on_event(Widget *self, int code) {
    if (code == 1) {
        self->closed = 1;
        widget_destroy(self);
    }
}

static char *app_build_status(const char *text) {
    char *msg = malloc(sizeof(Widget));   
    if (!msg) exit(1);

    memset(msg, 0xAB, sizeof(Widget));
    snprintf(msg, sizeof(Widget), "STATUS: %s", text);
    return msg;
}

int main(void) {
    Screen s = { .count = 0 };

    screen_add(&s, widget_new(&LABEL_VT,  10, "Welcome"));
    screen_add(&s, widget_new(&BUTTON_VT, 11, "OK"));
    screen_add(&s, widget_new(&DIALOG_VT, 12, "Are you sure?"));  /* items[2] */
    screen_add(&s, widget_new(&BUTTON_VT, 13, "Cancel"));

    printf("frame 1:\n");
    screen_render(&s);
    screen_dispatch(&s, 1);

    /* TODO 닫힌(closed) 위젯을 여기서 정리(free + 해당 슬롯 NULL)할 필요가 있음 */

    char *status = app_build_status("dialog closed");
    printf("%s\n", status);


    printf("frame 2:\n");
    screen_render(&s);           // 여기서 부름

    free(status);
    for (int i = 0; i < s.count; i++) free(s.items[i]);
    return 0;
}
