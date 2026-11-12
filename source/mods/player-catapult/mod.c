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
#include "../../runtime/native/local_actor.h"
#include "settings.h"
int _fltused=0;
void *memset(void *to,int value,U64 count){unsigned char *p=to;while(count--)*p++=(unsigned char)value;return to;}
static void config_initialize(void){cfg_start();}static int configured_enabled(void){return cfg.enabled;}
static struct {SURoot root,arm,wood,iron;PTR force,body_field,grounded,passenger,riding,climbing,flying,kinematic;float cooldown,shot;SPVec3 floor,up,forward;int active,launch_uncertain,build_failed;} ct;
#include "launch.h"
static int feature_can_buy(PTR world){(void)world;return 1;}
static SURoot ct_material(SUColor color){
    int ok=1;void *shader_args[]={su.string("Universal Render Pipeline/Unlit")};
    PTR shader=call(su.shader_find,0,shader_args,&ok);
    if(!shader){ok=1;shader_args[0]=su.string("Sprites/Default");shader=call(su.shader_find,0,shader_args,&ok);}
    if(!ok||!shader)return 0;SURoot shader_root=su_hold(shader);if(!shader_root)return 0;
    PTR material=su_new("UnityEngine","Material");SURoot root=material?su_hold(material):0;
    if(!root){su_destroy(material);su_drop(&shader_root);return 0;}
    void *ctor_args[]={shader};call(su.material_ctor,material,ctor_args,&ok);
    if(ok){void *color_args[]={&color};call(su.material_color,material,color_args,&ok);}
    su_drop(&shader_root);if(!ok){su_destroy(material);su_drop(&root);}return root;
}
static PTR part(PTR tr,int shape,SPVec3 p,SPVec3 scale,int metal){PTR value=tt_primitive(tr,shape,p,scale,su_get(metal?ct.iron:ct.wood));if(!value)ct.build_failed=1;return value;}
static int ct_clear(SPVec3 a,SPVec3 b,float radius){int ok=1,mask=~(1<<2),trigger=1;void *q[]={&a,&b,&radius,&mask,&trigger};PTR result=call(su.physics_capsule,0,q,&ok);PTR raw=ok&&result?unbox(result):0;return raw&&!*(unsigned char *)raw;}
static int ct_floor(SPVec3 at,SPVec3 up,SPVec3 *floor){unsigned char hit[64];if(ct_ray(su_add(at,su_mul(up,2)),su_mul(up,-1),5,hit)!=1)return 0;SPVec3 normal;su_copy(floor,hit+su.hit_point,sizeof(*floor));su_copy(&normal,hit+su.hit_normal,sizeof(normal));return ct_vector_valid(*floor)&&ct_vector_valid(normal)&&su_dot(normal,up)>.95f;}
static void ct_remove(void){su_destroy(su_get(ct.root));su_drop(&ct.root);su_drop(&ct.arm);ct.active=0;ct.shot=0;}
static int ct_place(PTR walker){
    SPVec3 up,forward,position,floor;PTR player_transform=tt_transform(walker),camera_transform=tt_transform(su_get(tt.camera));
    if(!moon_local_actor_admit(walker)||!player_transform||!camera_transform||!ct_vector(su.tr_up,player_transform,&up)||!ct_vector(su.tr_forward,camera_transform,&forward)||!ct_vector(su.tr_get_position,player_transform,&position))return 0;
    forward=su_sub(forward,su_mul(up,su_dot(forward,up)));if(su_dot(forward,forward)<.0001f)return 0;
    forward=su_normal(forward);SPVec3 right=su_normal(su_cross(up,forward));
    if(!ct_floor(su_add(position,su_mul(forward,4.5f)),up,&floor))return 0;
    for(int i=0;i<4;i++){SPVec3 corner=su_add(floor,su_add(su_mul(right,i&1?1.3f:-1.3f),su_mul(forward,i&2?1.7f:-1.7f))),sample;if(!ct_floor(corner,up,&sample)||su_dot(su_sub(sample,floor),up)>.18f||su_dot(su_sub(sample,floor),up)<-.18f)return 0;}
    if(!ct_clear(su_add(floor,su_mul(up,.8f)),su_add(floor,su_mul(up,2.5f)),1.25f))return 0;
    if(!ct.wood)ct.wood=ct_material((SUColor){.30f,.17f,.075f,1});if(!ct.iron)ct.iron=ct_material((SUColor){.15f,.17f,.18f,1});
    if(!su_get(ct.wood)||!su_get(ct.iron))return 0;
    if(!tt_host()||!moon_local_actor_admit(walker))return 0;
    PTR root=su_gameobject("Timber player catapult",0);SURoot root_handle=root?su_hold(root):0;
    if(!root_handle){su_destroy(root);return 0;}PTR tr=su_transform(root);
    if(!tr){su_destroy(root);su_drop(&root_handle);return 0;}ct.build_failed=0;
    su_set_vector(su.tr_position,tr,floor);su_set_quat(su.tr_rotation,tr,su_rotation(forward,up));
    for(int s=-1;s<=1;s+=2){part(tr,3,su_v(s*1.1f,.12f,0),su_v(.22f,.22f,3.5f),0);for(int z=-1;z<=1;z+=2){PTR leg=part(tr,3,su_v(s*.75f,.8f,z*.65f),su_v(.18f,1.8f,.18f),0);if(!leg)continue;SPVec3 aim=su_normal(su_v(-s*.45f,1,-z*.4f));su_set_quat(su.tr_local_rotation,su_transform(leg),su_rotation(su_v(0,0,1),aim));}}
    part(tr,3,su_v(0,.15f,1.35f),su_v(2.4f,.2f,.2f),0);part(tr,3,su_v(0,.15f,-1.35f),su_v(2.4f,.2f,.2f),0);part(tr,3,su_v(0,1.15f,0),su_v(2.1f,.22f,.22f),1);
    PTR pivot=su_gameobject("Catapult throwing arm",tr);SURoot arm_handle=pivot?su_hold(pivot):0;PTR arm=pivot?su_transform(pivot):0;
    if(!arm_handle||!arm){su_destroy(root);su_drop(&root_handle);su_drop(&arm_handle);return 0;}su_set_vector(su.tr_local_position,arm,su_v(0,1.15f,0));
    part(arm,3,su_v(0,0,-.25f),su_v(.23f,.23f,3.3f),0);part(arm,3,su_v(0,-.12f,-1.6f),su_v(.95f,.16f,.95f),0);part(arm,3,su_v(0,.08f,-2.02f),su_v(.95f,.4f,.12f),0);for(int s=-1;s<=1;s+=2)part(arm,3,su_v(s*.43f,.08f,-1.62f),su_v(.12f,.4f,.85f),0);
    part(tr,3,su_v(0,.45f,1.25f),su_v(.65f,.65f,.55f),1);
    if(ct.build_failed){su_destroy(root);su_drop(&root_handle);su_drop(&arm_handle);return 0;}
    /* Build the candidate before discarding the previous emplacement. */
    ct_remove();ct.root=root_handle;ct.arm=arm_handle;ct.floor=floor;ct.up=up;ct.forward=forward;ct.active=1;tt_event("catapult-placed");return 1;
}
static void feature_drop(void){ct_remove();su_destroy(su_get(ct.wood));su_destroy(su_get(ct.iron));su_drop(&ct.wood);su_drop(&ct.iron);ct.cooldown=ct.shot=0;}
static void feature_tick(PTR walker,PTR world,int allow){
    (void)world;if(!tt_host()){if(ct.active)ct_remove();return;}ct.cooldown-=tt.dt;if(ct.shot>0){ct.shot+=tt.dt;float t=ct.shot<.3f?ct.shot/.3f:1-(ct.shot-.3f)/1.2f;if(t<0){t=0;ct.shot=0;}SPVec3 axis=su_v(1,0,0);float angle=-1.15f*t;SUQuat q={axis.x*angle*.5f,0,0,1};float length=su_sqrt(q.x*q.x+q.w*q.w);q.x/=length;q.w/=length;if(ct.arm)su_set_quat(su.tr_local_rotation,su_transform(su_get(ct.arm)),q);}
    int input_allowed=allow&&tt.owned&&tt_host();MoonInputState place=tt_action("place",112,input_allowed),fire=tt_action("fire",cfg.action,input_allowed),pack=tt_action("pack",111,input_allowed);
    if(input_allowed){if(place.pressed){if(ct_place(walker))tt.status="Placed; walk to the rear bucket and press E";else tt.status="Aim over clear, level ground 4.5 m ahead";}if(fire.pressed)ct_fire(walker);if(pack.pressed){ct_remove();tt.status="Catapult packed";}}
    if(ct.launch_uncertain)tt.status="Native launch outcome unknown; restart before another launch";
    char b[350],*p=cat(b,"P: place  |  E: launch at rear bucket  |  O: pack\n");p=cat(p,tt.status?tt.status:"Timber catapult; one active emplacement per session");tt_shop_text(b);
}
static int ct_update(int id,Context *context){
    /* Shared Update observes every walker. Remote callbacks must not consume
     * this provider's frame latch or replace its retained scene. */
    if(!context||!moon_local_actor_admit((PTR)context->rcx))return 0;
    return tt_update(id,context);
}
static int ct_owned(PTR world,PTR walker){return moon_local_actor_admit(walker)&&tt_is_owned(world,walker);}
static unsigned ct_offer(PTR world,PTR walker){return moon_local_actor_admit(walker)?tt_offer_state(world,walker):MS_BLOCKED;}
static int ct_purchase(PTR world,PTR walker){return moon_local_actor_admit(walker)?tt_purchase(world,walker):0;}
static int ct_install(PTR game){
    MoonEquipmentDefinition equipment={ME_ABI,sizeof(equipment),TOOL_ID,TOOL_NAME,TOOL_DESCRIPTION,0,ct_owned,tt_configure};
    MoonShopDefinition offer={MS_ABI,sizeof(offer),TOOL_ID,TOOL_NAME,TOOL_DESCRIPTION,TOOL_PRICE,ct_offer,ct_purchase};
    if(!tt.equipment->register_item(&equipment)||!tt.shop->register_item(&offer))return 0;
    if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;
    before_hook=ct_update;if(!install_hooks())return 0;tt_event("ready");return 1;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(!tt_start(domain,game))return 0;ct.force=su_method("UnityEngine","Rigidbody","AddForce","UnityEngine.Vector3;UnityEngine.ForceMode");ct.body_field=get_field(find_class("MoonCleaners","MoonWalker"),"body");
    ct.kinematic=su_method("UnityEngine","Rigidbody","get_isKinematic","");
    ct.grounded=su_method("MoonCleaners","MoonWalker","set_Grounded","System.Boolean");ct.passenger=su_method("MoonCleaners","MoonWalker","get_Passenger","");ct.riding=su_method("MoonCleaners","MoonWalker","get_Riding","");ct.climbing=su_method("MoonCleaners","MoonWalker","get_Climbing","");ct.flying=su_method("MoonCleaners","MoonWalker","get_Flying","");
    PTR (*field_type)(PTR)=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_field_get_type");
    PTR mode_class=find_class("UnityEngine","ForceMode"),mode=mode_class?get_field(mode_class,"VelocityChange"):0;int velocity_change=-1;
    if(mode)su.static_field(mode,&velocity_change);
    char *type=field_type&&ct.body_field?su.type_name(field_type(ct.body_field)):0;
    int body_valid=type&&su_eq(type,"UnityEngine.Rigidbody");if(type)su.free(type);
    if(!body_valid||!ct.force||!ct.kinematic||!ct.grounded||method_token(ct.grounded)!=100671645||!ct.passenger||!ct.riding||!ct.climbing||!ct.flying||velocity_change!=CT_VELOCITY_CHANGE||!moon_local_actor_bind())return 0;
    return ct_install(game);
}
