#include "../../runtime/native/gameplay.h"
#define TOOL_ID "player-catapult"
#define TOOL_DESCRIPTION "Portable timber player launcher. Select in Equipment; P places, E launches at the rear bucket and O packs."
#define TOOL_NAME "Player Catapult"
#define TOOL_PRICE 750
#define TOOL_KEY 121
#define TOOL_ACTION 101
#define TOOL_SPEED 38.f
#define TOOL_ENV "MOONLAB_PLAYER_CATAPULT"
#define TOOL_HOTKEY "Alt+Y"
#define TOOL_ROW 1
#include "tool_runtime.h"
#include "settings.h"
int _fltused=0;
void *memset(void *to,int value,U64 count){unsigned char *p=to;while(count--)*p++=(unsigned char)value;return to;}
static void config_initialize(void){cfg_start();}static int configured_enabled(void){return cfg.enabled;}
static struct {SURoot root,arm,wood,iron;PTR force,body_field,grounded,passenger,riding,climbing,flying,field_type;float cooldown,shot;SPVec3 floor,up,forward;int active;} ct;
static int feature_can_buy(PTR world){(void)world;return 1;}
static PTR part(PTR tr,int shape,SPVec3 p,SPVec3 scale,int metal){return tt_primitive(tr,shape,p,scale,su_get(metal?ct.iron:ct.wood));}
static int ct_clear(SPVec3 a,SPVec3 b,float radius){int ok=1,mask=~(1<<2),trigger=1;void *q[]={&a,&b,&radius,&mask,&trigger};PTR result=call(su.physics_capsule,0,q,&ok);return ok&&result&&!*(unsigned char *)unbox(result);}
static int ct_floor(SPVec3 at,SPVec3 up,SPVec3 *floor){unsigned char hit[64];if(!tt_ray(su_add(at,su_mul(up,2)),su_mul(up,-1),5,hit))return 0;SPVec3 normal;su_copy(floor,hit+su.hit_point,sizeof(*floor));su_copy(&normal,hit+su.hit_normal,sizeof(normal));return su_dot(normal,up)>.95f;}
static void ct_remove(void){su_destroy(su_get(ct.root));su_drop(&ct.root);su_drop(&ct.arm);ct.active=0;}
static int ct_place(PTR walker){
    SPVec3 up=su_vector(su.tr_up,tt_transform(walker)),forward=su_vector(su.tr_forward,tt_transform(su_get(tt.camera)));forward=su_normal(su_sub(forward,su_mul(up,su_dot(forward,up))));SPVec3 right=su_normal(su_cross(up,forward)),position=su_vector(su.tr_get_position,tt_transform(walker)),floor;
    if(!ct_floor(su_add(position,su_mul(forward,4.5f)),up,&floor))return 0;
    for(int i=0;i<4;i++){SPVec3 corner=su_add(floor,su_add(su_mul(right,i&1?1.3f:-1.3f),su_mul(forward,i&2?1.7f:-1.7f))),sample;if(!ct_floor(corner,up,&sample)||su_dot(su_sub(sample,floor),up)>.18f||su_dot(su_sub(sample,floor),up)<-.18f)return 0;}
    if(!ct_clear(su_add(floor,su_mul(up,.8f)),su_add(floor,su_mul(up,2.5f)),1.25f))return 0;
    ct_remove();ct.floor=floor;ct.up=up;ct.forward=forward;
    if(!ct.wood)ct.wood=su_hold(tt_material((SUColor){.30f,.17f,.075f,1}));if(!ct.iron)ct.iron=su_hold(tt_material((SUColor){.15f,.17f,.18f,1}));
    PTR root=su_gameobject("Timber player catapult",0),tr=su_transform(root);ct.root=su_hold(root);su_set_vector(su.tr_position,tr,floor);su_set_quat(su.tr_rotation,tr,su_rotation(forward,up));
    for(int s=-1;s<=1;s+=2){part(tr,3,su_v(s*1.1f,.12f,0),su_v(.22f,.22f,3.5f),0);for(int z=-1;z<=1;z+=2){PTR leg=part(tr,3,su_v(s*.75f,.8f,z*.65f),su_v(.18f,1.8f,.18f),0);SPVec3 aim=su_normal(su_v(-s*.45f,1,-z*.4f));su_set_quat(su.tr_local_rotation,su_transform(leg),su_rotation(su_v(0,0,1),aim));}}
    part(tr,3,su_v(0,.15f,1.35f),su_v(2.4f,.2f,.2f),0);part(tr,3,su_v(0,.15f,-1.35f),su_v(2.4f,.2f,.2f),0);part(tr,3,su_v(0,1.15f,0),su_v(2.1f,.22f,.22f),1);
    PTR pivot=su_gameobject("Catapult throwing arm",tr);ct.arm=su_hold(pivot);PTR arm=su_transform(pivot);su_set_vector(su.tr_local_position,arm,su_v(0,1.15f,0));
    part(arm,3,su_v(0,0,-.25f),su_v(.23f,.23f,3.3f),0);part(arm,3,su_v(0,-.12f,-1.6f),su_v(.95f,.16f,.95f),0);part(arm,3,su_v(0,.08f,-2.02f),su_v(.95f,.4f,.12f),0);for(int s=-1;s<=1;s+=2)part(arm,3,su_v(s*.43f,.08f,-1.62f),su_v(.12f,.4f,.85f),0);
    part(tr,3,su_v(0,.45f,1.25f),su_v(.65f,.65f,.55f),1);ct.active=1;tt_event("catapult-placed");return 1;
}
static void ct_fire(PTR walker){
    int ok=1;if(!ct.active||ct.cooldown>0||!tt_host())return;SPVec3 position=su_vector(su.tr_get_position,tt_transform(walker)),bucket=su_add(ct.floor,su_add(su_mul(ct.up,1.05f),su_mul(ct.forward,-1.6f)));
    if(su_dot(su_sub(position,bucket),su_sub(position,bucket))>2.25f){tt.status="Stand at the rear bucket, within 1.5 m";return;}
    if(boolean(ct.passenger,walker,&ok)||call(ct.riding,walker,0,&ok)||boolean(ct.climbing,walker,&ok)||boolean(ct.flying,walker,&ok)||!ok){tt.status="Leave vehicles, robots, ladders and flight first";return;}
    SPVec3 direction=su_normal(su_add(su_mul(ct.forward,.8f),su_mul(ct.up,.65f)));unsigned char hit[64];
    for(int i=0;i<3;i++){SPVec3 offset=su_mul(su_normal(su_cross(ct.up,ct.forward)),(i-1)*.38f);if(tt_ray(su_add(su_add(position,offset),su_mul(ct.up,.4f)),direction,4,hit)){tt.status="Launch path obstructed";return;}}
    PTR body=0;field_value(walker,ct.body_field,&body);if(!su_alive(body)){tt.status="Player physics body unavailable";return;}
    SPVec3 velocity=su_mul(direction,cfg.speed);int force_mode=2;void *args[]={&velocity,&force_mode};call(ct.force,body,args,&ok);if(!ok){tt.status="Physics launch refused";return;}
    su_bool(ct.grounded,walker,0);ct.cooldown=4;ct.shot=.001f;tt.status="Launched; normal gravity and collisions remain active";tt_event("player-launched");
}
static void feature_drop(void){ct_remove();su_destroy(su_get(ct.wood));su_destroy(su_get(ct.iron));su_drop(&ct.wood);su_drop(&ct.iron);ct.cooldown=ct.shot=0;}
static void feature_tick(PTR walker,PTR world,int allow){
    (void)world;ct.cooldown-=tt.dt;if(ct.shot>0){ct.shot+=tt.dt;float t=ct.shot<.3f?ct.shot/.3f:1-(ct.shot-.3f)/1.2f;if(t<0){t=0;ct.shot=0;}SPVec3 axis=su_v(1,0,0);float angle=-1.15f*t;SUQuat q={axis.x*angle*.5f,0,0,1};float length=su_sqrt(q.x*q.x+q.w*q.w);q.x/=length;q.w/=length;if(ct.arm)su_set_quat(su.tr_local_rotation,su_transform(su_get(ct.arm)),q);}
    int input_allowed=allow&&tt.owned&&tt_host();MoonInputState place=tt_action("place",112,input_allowed),fire=tt_action("fire",cfg.action,input_allowed),pack=tt_action("pack",111,input_allowed);
    if(input_allowed){if(place.pressed){if(ct_place(walker))tt.status="Placed; walk to the rear bucket and press E";else tt.status="Aim over clear, level ground 4.5 m ahead";}if(fire.pressed)ct_fire(walker);if(pack.pressed){ct_remove();tt.status="Catapult packed";}}
    char b[350],*p=cat(b,"P: place  |  E: launch at rear bucket  |  O: pack\n");p=cat(p,tt.status?tt.status:"Timber catapult; one active emplacement per session");tt_shop_text(b);
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(!tt_start(domain,game))return 0;ct.force=su_method("UnityEngine","Rigidbody","AddForce","UnityEngine.Vector3;UnityEngine.ForceMode");ct.body_field=get_field(find_class("MoonCleaners","MoonWalker"),"body");
    ct.grounded=su_method("MoonCleaners","MoonWalker","set_Grounded","System.Boolean");ct.passenger=su_method("MoonCleaners","MoonWalker","get_Passenger","");ct.riding=su_method("MoonCleaners","MoonWalker","get_Riding","");ct.climbing=su_method("MoonCleaners","MoonWalker","get_Climbing","");ct.flying=su_method("MoonCleaners","MoonWalker","get_Flying","");
    return ct.force&&ct.body_field&&ct.grounded&&method_token(ct.grounded)==100671645&&ct.passenger&&ct.riding&&ct.climbing&&ct.flying&&tt_install(game);
}
