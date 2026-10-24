#ifndef GF2VR_EVENT_LINE_H
#define GF2VR_EVENT_LINE_H
/* Same bytes as the former per-call recorder, including hex width/key order. */
static unsigned event_line(char *line,const char *kind,U64 count,DWORD tid,
                           DWORD pid,U64 tick,DWORD first,U64 base) {
    char *p=literal(line,"{\"probe\":\"" GF2VR_OBSERVER_ID "\",\"event\":\"");
    p=literal(p,kind);p=literal(p,"\",\"pid\":");p=decimal(p,pid);
    p=literal(p,",\"tid\":");p=decimal(p,tid);p=literal(p,",\"count\":");p=hexadecimal(p,count);
    p=literal(p,",\"tick_ms\":");p=hexadecimal(p,tick);p=literal(p,",\"first_tid\":");p=decimal(p,first);
    p=literal(p,",\"unity_base\":");p=hexadecimal(p,base);p=literal(p,GF2VR_ACTIVITY_FIELD);
    return (unsigned)(p-line);
}
#endif
