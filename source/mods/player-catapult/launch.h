/* Native AddForce owns the impulse. No movement hook or replacement gravity
 * loop is installed; an uncertain native write is never automatically retried. */
#define CT_VELOCITY_CHANGE 2
static int ct_finite(float value){return value==value&&value<=3.402823466e38f&&value>=-3.402823466e38f;}
static int ct_vector_valid(SPVec3 value){return ct_finite(value.x)&&ct_finite(value.y)&&ct_finite(value.z);}
static int ct_bool(PTR method,PTR target,int *value){
    int ok=1;PTR box=call(method,target,0,&ok),raw=ok&&box?unbox(box):0;
    if(!raw)return 0;*value=*(unsigned char *)raw!=0;return 1;
}
static int ct_vector(PTR method,PTR target,SPVec3 *value){
    int ok=1;PTR box=call(method,target,0,&ok),raw=ok&&box?unbox(box):0;
    if(!raw)return 0;su_copy(value,raw,sizeof(*value));return ct_vector_valid(*value);
}
/* A failed raycast is unknown, never evidence that a corridor is empty. */
static int ct_ray(SPVec3 origin,SPVec3 direction,float range,unsigned char *hit){
    int ok=1,mask=~(1<<2),trigger=1;su_zero(hit,64);
    void *args[]={&origin,&direction,hit,&range,&mask,&trigger};
    PTR box=call(su.physics_ray,0,args,&ok),raw=ok&&box?unbox(box):0;
    return raw?(*(unsigned char *)raw!=0):-1;
}
static void ct_fire(PTR walker){
    if(!ct.active||ct.cooldown>0||ct.launch_uncertain||!tt_host()||!moon_local_actor_admit(walker))return;
    SPVec3 position;PTR transform=tt_transform(walker);
    if(!transform||!ct_vector(su.tr_get_position,transform,&position)){tt.status="Player position unavailable";return;}
    SPVec3 bucket=su_add(ct.floor,su_add(su_mul(ct.up,1.05f),su_mul(ct.forward,-1.6f)));
    if(su_dot(su_sub(position,bucket),su_sub(position,bucket))>2.25f){tt.status="Stand at the rear bucket, within 1.5 m";return;}
    int passenger,climbing,flying,ok=1;
    if(!ct_bool(ct.passenger,walker,&passenger)||!ct_bool(ct.climbing,walker,&climbing)||!ct_bool(ct.flying,walker,&flying)){tt.status="Player movement state unavailable";return;}
    PTR riding=call(ct.riding,walker,0,&ok);
    if(!ok||passenger||riding||climbing||flying){tt.status="Leave vehicles, robots, ladders and flight first";return;}
    PTR body=0;field_value(walker,ct.body_field,&body);int kinematic;
    if(!su_alive(body)||!ct_bool(ct.kinematic,body,&kinematic)||kinematic){tt.status="Player physics body unavailable or parked";return;}
    SURoot body_root=su_hold(body);if(!body_root){tt.status="Player physics root unavailable";return;}
    SPVec3 direction=su_normal(su_add(su_mul(ct.forward,.8f),su_mul(ct.up,.65f)));unsigned char hit[64];
    for(int i=0;i<3;i++){
        SPVec3 offset=su_mul(su_normal(su_cross(ct.up,ct.forward)),(i-1)*.38f);
        int result=ct_ray(su_add(su_add(position,offset),su_mul(ct.up,.4f)),direction,4,hit);
        if(result){tt.status=result<0?"Launch path check unavailable":"Launch path obstructed";goto done;}
    }
    /* Recheck the exact physics target after the native preflight queries. */
    PTR current=0;field_value(walker,ct.body_field,&current);
    if(current!=body||!moon_local_actor_admit(walker)||!tt_host()||!ct_bool(ct.kinematic,body,&kinematic)||kinematic){tt.status="Player physics context changed";goto done;}
    SPVec3 velocity=su_mul(direction,cfg.speed);int mode=CT_VELOCITY_CHANGE;unsigned char no=0;
    void *ground_args[]={&no};call(ct.grounded,walker,ground_args,&ok);
    if(ok){void *force_args[]={&velocity,&mode};call(ct.force,body,force_args,&ok);}
    if(!ok){ct.launch_uncertain=1;tt.status="Native launch outcome unknown; restart before another launch";tt_event("launch-outcome-unknown");goto done;}
    ct.cooldown=4;ct.shot=.001f;tt.status="Launch requested; normal gravity and collisions remain active";tt_event("player-launch-requested");
done:
    su_drop(&body_root);
}
