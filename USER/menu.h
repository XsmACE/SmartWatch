#ifndef __MENU_H__
#define __MENU_H__

typedef struct __menu_t
{
    void(*item)(void);		    //��ǰ��ĿҪ��ʾ���ַ�
	const uint8_t *pic;			//��ǰ��ĿҪ��ʾ��ͼ��
    void(*fun)(void *);			//ѡ��ĳһ�˵���ִ�еĹ��ܺ���
    struct __menu_t *same_left; //��ǰ��Ŀ��ͬ�����˵�
    struct __menu_t *same_right;//��ǰ��Ŀ��ͬ���Ҳ�˵�	
    struct __menu_t *parent;	//��ǰ��Ŀ�ĸ��˵�	
    struct __menu_t *child;		//��ǰ��Ŀ���Ӳ˵�
}menu_t;


typedef enum __key_fn_t
{
	KEY_NONE=0,	
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_BACK,
}key_fn_t;

 typedef struct __menu_ext_t
{
	menu_t 	*menu;
	uint32_t item_cursor;
	uint32_t item_total;
	key_fn_t key_fn;	
}menu_ext_t;

extern void menu_show(menu_ext_t *menu_ext);
extern uint32_t menu_item_total(menu_t *menu);
extern menu_t menu_main_1[];
extern menu_t *menu_main_tbl[];


#endif
