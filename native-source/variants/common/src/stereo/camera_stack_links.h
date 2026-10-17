#ifndef GF2_CAMERA_STACK_LINKS_H
#define GF2_CAMERA_STACK_LINKS_H
/* Owner-thread preparation only. Adapter validates actual camera/list types and
   generation; invokes managed methods/GC barriers, never object-layout writes.
   No rendering authority. On failure DISCARD private group after its real
   borrows drain: partially prepared lists must never be published or reused.
   Sources and private cameras must remain owner-rooted beyond this call. */
#define CSL_CAPACITY 8u
typedef struct {void *source,*left,*right;} CameraStackMember;
typedef struct {
    void *context;
    int (*valid)(void *);
    unsigned (*pin)(void *,void *);
    void (*unpin)(void *,unsigned);
    int (*role)(void *,void *,unsigned *); /* 0 Base, 1 Overlay */
    int (*list)(void *,void *,unsigned,void **); /* cameras=0, stack=1 */
    int (*count)(void *,void *,unsigned *);
    int (*item)(void *,void *,unsigned,void **);
    int (*clear)(void *,void *);
    int (*add)(void *,void *,void *);
} CameraStackLinkApi;
typedef struct {void *list,*items[CSL_CAPACITY];unsigned count;} CameraStackList;
/* Caller-owned scratch keeps the no-CRT production hook below a stack page.
   One operation owns this storage; recursive use fails without clearing it. */
typedef struct {
    CameraStackList lists[CSL_CAPACITY][3][2];
    unsigned roots[CSL_CAPACITY*9],busy;
} CameraStackScratch;
static int csl_matches(const CameraStackLinkApi *a,const CameraStackList *s) {
    if(!s->list)return !s->count;
    unsigned n=0;if(!a->count(a->context,s->list,&n) || n!=s->count)return 0;
    for(unsigned i=0;i<n;++i) {
        void *item=0;if(!a->item(a->context,s->list,i,&item) || item!=s->items[i])return 0;
    }
    return a->valid(a->context);
}
/* Supports one Base with an ordered, flat Overlay group. Every supplied member
   must be referenced exactly once by both source lists. Nested stacks, UI and
   unexplained members are rejected, not silently flattened or omitted. */
static int camera_stack_links_prepare_with(const CameraStackLinkApi *a,
    const CameraStackMember *m,unsigned count,CameraStackScratch *scratch,unsigned refresh) {
    if(!a || !m || !count || count>CSL_CAPACITY || !a->valid || !a->pin || !a->unpin ||
       !a->role || !a->list || !a->count || !a->item || !a->clear || !a->add || !a->valid(a->context) ||
       !scratch || scratch->busy)return 0;
    scratch->busy=1;
    CameraStackList (*lists)[3][2]=scratch->lists;
    unsigned *roots=scratch->roots,nr=0;int ok=0;
    /* Validate disjoint object identities before querying or mutating. */
    for(unsigned i=0;i<count;++i) {
        void *v[]={m[i].source,m[i].left,m[i].right};
        for(unsigned j=0;j<3;++j) {
            if(!v[j])goto done;
            for(unsigned k=0;k<=i;++k) {
                void *p[]={m[k].source,m[k].left,m[k].right};
                for(unsigned l=0;l<3;++l)if((k<i || l<j) && v[j]==p[l])goto done;
            }
        }
    }
    for(unsigned i=0;i<count;++i) {
        void *v[]={m[i].source,m[i].left,m[i].right};
        for(unsigned j=0;j<3;++j) {
            unsigned root=a->pin(a->context,v[j]);if(!root)goto done;roots[nr++]=root;
            unsigned role=99;
            if(!a->role(a->context,v[j],&role) || role!=(i?1u:0u))goto done;
            for(unsigned l=0;l<2;++l) {
                CameraStackList *s=&lists[i][j][l];
                if(!a->list(a->context,v[j],l,&s->list))goto done;
                if(!s->list)continue;
                root=a->pin(a->context,s->list);if(!root)goto done;roots[nr++]=root;
                /* No source/private/cross-eye/cross-member aliasing. Aliasing
                   cameras/stack within the SAME component is allowed. */
                for(unsigned k=0;k<=i;++k)for(unsigned e=0;e<3;++e)for(unsigned b=0;b<2;++b)
                    if((k<i || e<j) && lists[k][e][b].list==s->list)goto done;
                if(!a->count(a->context,s->list,&s->count) || s->count>=CSL_CAPACITY)goto done;
                for(unsigned n=0;n<s->count;++n)
                    if(!a->item(a->context,s->list,n,&s->items[n]) || !s->items[n])goto done;
            }
        }
    }
    /* Full preflight before the first private write. */
    for(unsigned i=0;i<count;++i)for(unsigned l=0;l<2;++l) {
        CameraStackList *s=&lists[i][0][l];
        if(s->count!=(i?0u:count-1))goto done;
        for(unsigned eye=1;eye<3;++eye) {
            if((lists[i][eye][l].list!=0)!=(s->list!=0) ||
               lists[i][eye][l].count!=(refresh?s->count:0u))goto done;
            if(refresh)for(unsigned n=0;n<s->count;++n) {
                unsigned k=1;for(;k<count;++k)if(m[k].source==s->items[n])break;
                if(k==count || lists[i][eye][l].items[n]!=(eye==1?m[k].left:m[k].right))goto done;
            }
            if((lists[i][eye][0].list==lists[i][eye][1].list)!=
               (lists[i][0][0].list==lists[i][0][1].list))goto done;
        }
        unsigned seen=0;
        for(unsigned n=0;n<s->count;++n) {
            unsigned k=1;for(;k<count;++k)if(m[k].source==s->items[n])break;
            if(k==count || (seen&(1u<<k)))goto done;seen|=1u<<k;
            if(s->items[n]!=lists[i][0][1-l].items[n])goto done;
        }
        if(!csl_matches(a,s))goto done;
    }
    for(unsigned eye=1;!refresh && eye<3;++eye)for(unsigned l=0;l<2;++l) {
        CameraStackList *s=&lists[0][0][l],*d=&lists[0][eye][l];
        if(!d->list || (l && d->list==lists[0][eye][0].list))continue;
        if(!a->valid(a->context) || !a->clear(a->context,d->list))goto done;
        for(unsigned n=0;n<s->count;++n) {
            unsigned k=1;for(;k<count;++k)if(m[k].source==s->items[n])break;
            void *clone=eye==1?m[k].left:m[k].right;
            if(!a->valid(a->context) || !a->add(a->context,d->list,clone))goto done;
        }
    }
    /* Verify both the original snapshot and exact eye-local remapping. */
    for(unsigned i=0;i<count;++i)for(unsigned l=0;l<2;++l) {
        CameraStackList *s=&lists[i][0][l];if(!csl_matches(a,s))goto done;
        for(unsigned eye=1;eye<3;++eye) {
            CameraStackList expected=lists[i][eye][l];expected.count=s->count;
            for(unsigned n=0;n<s->count;++n) {
                unsigned k=1;for(;k<count;++k)if(m[k].source==s->items[n])break;
                expected.items[n]=eye==1?m[k].left:m[k].right;
            }
            if(!csl_matches(a,&expected))goto done;
        }
    }
    ok=a->valid(a->context);
done:
    while(nr)a->unpin(a->context,roots[--nr]);
    for(unsigned i=0;i<CSL_CAPACITY;++i)for(unsigned e=0;e<3;++e)for(unsigned l=0;l<2;++l)
        lists[i][e][l]=(CameraStackList){0};
    for(unsigned i=0;i<CSL_CAPACITY*9;++i)roots[i]=0;
    scratch->busy=0;return ok;
}
static int camera_stack_links_prepare(const CameraStackLinkApi *a,const CameraStackMember *m,unsigned count) {
    CameraStackScratch scratch={0};return camera_stack_links_prepare_with(a,m,count,&scratch,0);
}
#endif
