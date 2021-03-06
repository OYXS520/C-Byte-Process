/**
 * @brief  用于解决tcp传输时的粘包问题
 * @author shuouyang
 * @e-mail 
 *  
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "byte_process.h"
/**
 * @brief  获取文件大小
 * @note   单位byte（字节)
 * @param  *filepath: 文件路径
 * @retval 
 */
size_t get_file_size(const char *filepath)
{
    /*check input para*/
    if (NULL == filepath)
        return 0;
    struct stat filestat;
    memset(&filestat, 0, sizeof(struct stat));
    /*get file information*/
    if (0 == stat(filepath, &filestat))
        return filestat.st_size;
    else
        return 0;
}
/**
 * @brief  初始化bytes数组
 * @note   
 * @retval 
 */
bytes *init_bytes()
{
    bytes *init = (bytes *)malloc(sizeof(bytes));
    init->len = 0;
    init->data = NULL;
    return init;
}
/**
 *! @brief  创建字符数组
 * @note   copy原始数组中的数据，用完需释放 使用@free_bytes
 * @param  *data: 字节数组
 * @param  len: 字节数组的长度
 * @retval 
 */
bytes *create_bytes(byte *data, int len)
{
    bytes *res = init_bytes();
    res->data = (byte *)malloc(len);
    memcpy(res->data, data, len);
    res->len = len;
    return res;
}
/**
 * @brief  打印字节数组
 * @note   
 * @param  *src: 
 * @param  len: 
 * @retval None
 */
void print_byte(byte *src, int len)
{
    printf("len = %d | data = ", len);
    printf("[");
    int i;
    for (i = 0; i < len - 1; i++)
    {
        printf("%.2x,", src[i]);
    }
    if (src != NULL)
    {
        printf("%.2x", src[i]);
    }
    printf("]\n");
}
/**
 *! @brief  打印字节数组
 * @note   
 * @param  *src: 
 * @retval None
 */
void print_bytes(bytes *src)
{
    if (src == NULL)
    {
        perror("bytes is NULL");
    }
    print_byte(src->data, src->len);
}
/**
 *! @brief  释放字节数组
 * @note   
 * @param  *src: 
 * @retval None
 */
void free_bytes(bytes *src)
{
    free(src->data);
    src->data = NULL;
    free(src);
    src = NULL;
}
/**
 * @brief  释放字节数组的数组
 * @note   
 * @param  **src: 
 * @retval None
 */
void free_bytes_array(bytes **src, int len)
{
    for (size_t i = 0; i < len; i++)
    {
        free_bytes(src[i]);
    }
    free(src);
}
/**
 * @brief  释放包信息结构体
 * @note   
 * @param  *info: 
 * @retval None
 */
void free_tcp_package_info(tcp_package_info *info)
{
    free_bytes(info->pag_tail);
    info->pag_tail = NULL;
    free_bytes(info->pag_head);
    info->pag_head = NULL;
    free(info);
    info = NULL;
}

/**
 *! @brief  安全分配内存
 * @note   
 * @param  *src: 
 * @param  new_size: 
 * @retval None
 */
void realloc_security(byte **src, int new_size)
{
    if (*src == NULL)
    {
        printf("realloc_security:源字节数组为空,直接申请内存\n");
        *src = (byte *)malloc(sizeof(byte) * new_size);
    }
    else
    {
        byte *new_ptr = realloc(*src, new_size);
        if (!new_ptr)
        {
            perror("realloc failed!!!");
        }
        *src = new_ptr;
    }
}

/**
 *! @brief  将tail的前len个字节拼接到src末尾
 * @note   会在src的基础上扩充
 * @param  *src: 
 * @param  *tail: 
 * @param  len: 
 * @retval None
 */

void bytes_splicing(bytes *src, bytes *tail, int len)
{
    if (src == NULL)
    {
        perror("bytes is NULL");
    }
    // 先扩充原数组的大小
    int origin_len = src->len;
    realloc_security(&(src->data), src->len + len);
    src->len = src->len + len;
    memcpy(&(src->data[origin_len]), tail->data, len);
}

/**
 *! @brief  截取src字节数组，从start开始到end结束，包含start ，不包含end
 * @note   返回的bytes * 为复制内存生成，需自行释放内存
 * @param  *src: 
 * @param  start: 
 * @param  end: 
 * @retval None
 */
bytes *bytes_intercept(bytes *src, int start, int end)
{
    if (src == NULL)
    {
        perror("bytes_intercept ：bytes is NULL ");
    }
    if (start > src->len || end > src->len || end < start)
    {
        perror("index ineffective");
    }

    bytes *res = init_bytes();
    res->data = (byte *)malloc(end - start);
    memcpy(res->data, &(src->data[start]), (end - start));
    res->len = end - start;
    return res;
}
/**
 *! @brief  将tar与src比较。src大返回1，src小返回-1，相等返回0
 * @note   
 * @param  *src: 
 * @param  *tar: 
 * @retval 
 */
int bytes_compares(bytes *src, bytes *tar)
{
    if (src == NULL || tar == NULL)
    {
        perror("bytes_compares bytes is NULL");
        exit(1);
    }

    if (src->len > tar->len)
    {
        return 1;
    }
    else if (src->len < tar->len)
    {
        return -1;
    }
    else
    { //长度相等
        for (size_t i = 0; i < src->len; i++)
        {
            if (src->data[i] > tar->data[i])
            {
                return 1;
            }
            else if (src->data[i] < tar->data[i])
            {
                return -1;
            }
        }
        return 0;
    }
}
/**
 *! @brief  比较src和tar的前几位
 * @note   
 * @param  *src: 
 * @param  *tar: 
 * @retval 
 */
int bytes_n_compares(bytes *src,int start_1, bytes *tar,int start_2, int len)
{
    if (src == NULL || tar == NULL)
    {
        perror("bytes_compares bytes is NULL");
        exit(1);
    }
    for (size_t i = 0; i < len; i++)
    {
        if (src->data[start_1+i] > tar->data[start_2+i])
        {
            return 1;
        }
        else if (src->data[start_1+i] < tar->data[start_2+i])
        {
            return -1;
        }
    }
    return 0;
}
int bytes_search(bytes *src,bytes *tar){
    if (src == NULL || tar == NULL)
    {
        perror("bytes_search bytes is NULL");
    }
    if (src->data == NULL || tar->data == NULL)
    {
        printf("bytes_search bytes->data is NULL\n");
        return -1;
    }
    
    if (tar->len ==0)
    {
        return 0;
    }
    const byte *s1 = src->data;
	const byte *s2 = tar->data;
	const byte *cp = src->data;
    int res = 0;
    while (*cp)
    {
        s1 = cp;
        s2 = tar->data;
        while (*s1 && *s2 && *s1== *s2)
        {
           s1 ++;
           s2 ++;
        }
        if (!*s2)
        {
             return res;   
        }
        cp ++;
        res ++;
    }
    return -1;  
}
/**
 *! @brief  重要的全局配置信息
 * @note   
 * @retval None
 */
bytes *buffer_link = NULL;
volatile bytes_queue *package_queue = NULL;
volatile int package_queue_size = 0;
tcp_package_info *pack_info = NULL;
int current_data_len; //当前检查的数据的长度

int base_len = 0; //底线长度 包头字节长度+数据位长度

/**
 * @brief  int转字节数组
 * @note   只限整型
 * @param  num: 
 * @retval 
 */
bytes *int_to_bytes(int num)
{
    bytes *ret = init_bytes();
    ret->data = malloc(4);
    ret->data[0] = (byte)((num >> 24) & 0xFF);
    ret->data[1] = (byte)((num >> 16) & 0xFF);
    ret->data[2] = (byte)((num >> 8) & 0xFF);
    ret->data[3] = (byte)(num & 0xFF);
    ret->len = 4;
    return ret;
}
/**
 * @brief  字节数组转int
 * @note   只限四位字节
 * @param  *data: 
 * @retval 
 */
int bytes_to_int(bytes *data)
{
    int mask = 0xFF;
    int temp = 0;
    int res = 0;
    for (size_t i = 0; i < 4; i++)
    {
        res <<= 8;
        temp = (data->data[i]) & mask;
        res |= temp;
    }
    return res;
}
/**
 *! @brief  将数据位的长度转换为进制整数
 * @note   
 * @param  *data_bit: 
 * @retval 
 */
int parse_data_bit(bytes *data_bit)
{
    int len = data_bit->len - 1;
    int ret = 0;
    for (size_t i = 0; i <= len; i++)
    {
        ret = ret + data_bit->data[i] * pow(10, len - i);
    }
    return ret;
}

/**
 * @brief  解析整型到字节数组
 * @note   
 * @param  num: 数
 * @param  len: 转为字节数组的长度
 * @retval 
 */
bytes *parse_int_to_bytes(int num, int len)
{
    bytes *res = init_bytes();
    res->data = (byte *)malloc(len);
    res->len = len;
    int yu = num;
    for (int i = len - 1; i >= 0; i--)
    {
        res->data[len - 1 - i] = (byte)(yu / ((int)pow(10, i)));
        yu = yu % ((int)pow(10, i));
    }
    return res;
}
/**
 *! @brief  设置包头包尾信息
 * @note   
 * @param  *head:  包头
 * @param  head_size: 包头占字节长度
 * @param  *tail: 包尾
 * @param  tail_size: 包尾长度
 * @param  data_bit_len:数据位长度
 * @retval None
 */
void set_package_info(byte *head, int head_size, byte *tail, int tail_size, int data_bit_len)
{
    if (pack_info == NULL)
    {
        pack_info = (tcp_package_info *)malloc(sizeof(tcp_package_info));
    }
    pack_info->pag_head = create_bytes(head, head_size);
    pack_info->pag_tail = create_bytes(tail, tail_size);
    pack_info->data_bits = data_bit_len;
    base_len = head_size + data_bit_len;
    printf("set pack_info success !!!!\n");
}
/**
 *! @brief  打印包信息
 * @note   
 * @retval None
 */
void print_package_info()
{
    printf("package head :");
    print_bytes(pack_info->pag_head);
    printf("package tail :");
    print_bytes(pack_info->pag_tail);
    printf("package data bits len: %d\n", pack_info->data_bits);
}

/**
 *! @brief  为字节数组的数组添加字节数组
 * @note   
 * @param  ***bytess: 
 * @param  *tail: 
 * @param  bytess_len: 
 * @retval None
 */
void add_bytes_to_bytess(bytes ***array, bytes *tail, int array_len)
{
    if (*array == NULL)
    {
        *array = (bytes **)malloc(sizeof(bytes *)); //先分配一个地址内存
    }
    else
    {
        bytes **tem = realloc(*array, sizeof(bytes *) * (array_len + 1));
        if (tem == NULL)
        {
            printf("add_bytes_to_bytess bytes ** is NULL");
            exit(1);
        }
        *array = tem;
    }
    (*array)[array_len] = tail; //!运算符优先级
    void *check = &((*array)[array_len]);
}

/**
 * @brief  将读取到的buffer拼接成链式，然后从中查找已经完整的包
 * @note   
 * @param  *buffer: 
 * @param  **results: 
 * @retval 返回添加buffer后队列中新增加的完整包的数量
 */
int check_package_from_buffer_link(bytes *buffer)
{
    //TODO:主要逻辑
    // bytes *buffer_link = NULL;
    // tcp_package_info *pack_info = NULL;
    // int current_data_len; //当前检查的数据的长度
    int res = 0;   //查找到的完整包的个数
    if (buffer_link == NULL) //如果buffer_link没有初始化,先初始化
    {
        buffer_link = init_bytes();
        int index = bytes_search(buffer,pack_info->pag_head);//查找包头
        if (index < 0 )
        {
            printf("第一个包中没有包头\n");
            return res;
        }else if ( index  > 0)
        {   //如果第一个包中有包头，但是包头不在第一个，就丢弃包头之前的数据，拿取包头和包头之后的数据
            bytes * temp = init_bytes();
            temp = bytes_intercept(buffer,index,buffer->len);
            free_bytes(buffer);
            buffer = temp;
        }
        
    }  
    bytes_splicing(buffer_link, buffer, buffer->len); //将新的字节数组拼接到老的后面
    free_bytes(buffer);
jomp:

    if (buffer_link->len < base_len) //如果初始数据连包头和数据位的长度都没有就等下一次数据继续拼接
    {
        return res;
    }
    else
    {                                                                                          //肯定有完整的包头和长度位
        if (bytes_n_compares(buffer_link,0, pack_info->pag_head,0, pack_info->pag_head->len) == 0) //发现包头
        {
            bytes *data_bit = bytes_intercept(buffer_link, pack_info->pag_head->len, base_len); //截取数据位
            current_data_len = bytes_to_int(data_bit);
            free_bytes(data_bit);
            // printf("当前处理的数据长度为%d\n", current_data_len);
            int complete_package_len = current_data_len + base_len + pack_info->pag_tail->len;
            // printf("当前包的应该的完整长度%d\n",complete_package_len);
            if (buffer_link->len >= complete_package_len) //如果长度够了一个完整的包
            {

                if (bytes_n_compares(buffer_link,complete_package_len - pack_info->pag_tail->len ,pack_info->pag_tail, 0,pack_info->pag_tail->len) == 0)
                {
                    //1 复制出完整的包，
                    //2 将剩余的数据复制到新的buffer链中
                    //3 释放掉之前的buffer链
                    //4 重新定位buffer链
                    //获取数据    （复制）                                                  //包头长度加数据位长度                                      // 包头长度加数据位长度加数据长度
                    bytes *package = bytes_intercept(buffer_link, pack_info->pag_head->len + pack_info->data_bits, pack_info->pag_head->len + pack_info->data_bits + current_data_len);
                    add_bytes_to_queue(&package_queue,package);
                    res++;
                    printf("获取到一个包,当前是第%d个包\n", res);
                    //复制数据链中剩余的信息
                    bytes *tem = bytes_intercept(buffer_link, complete_package_len, buffer_link->len);
                    free_bytes(buffer_link);
                    // printf("打印获取到一个包信息的剩余数据");
                    buffer_link = tem; //将剩余的数据作为新的数据链
                    // print_bytes(buffer_link);
                    goto jomp; //再次检查数据链
                }
            }
            else
            { //长度不够一个完整的包
                return res;
            }
        }
    }
    return res;
}

/**
 * @brief  打印buffer link
 * @note   
 * @retval None
 */
void print_buffer_link()
{
    print_bytes(buffer_link);
}
/**
 * @brief  初始化队列接节点
 * @note   
 * @retval 
 */
bytes_queue *init_bytes_queue(){
    bytes_queue *res = (bytes_queue *)malloc(sizeof(bytes_queue));
    res->next = NULL;
    return res;
}
/**
 * @brief  释放单个节点的内存
 * @note   
 * @param  *note: 
 * @retval None
 */
void free_bytes_queue(bytes_queue *note){
    free_bytes(note->data);
    free(note);
    note = NULL;
}
/**
 * @brief  获取队列的大小
 * @note   
 * @param  *head: 
 * @retval 
 */
int bytes_queue_size(){
    return package_queue_size;
}
/**
 * @brief  打印队列
 * @note   
 * @param  *head: 
 * @retval None
 */
void print_bytes_queue(){
    bytes_queue *p = package_queue;
    if (p == NULL)
    {
        printf("队列为空\n");
        return;
    }
    
    for (size_t i = 0; p != NULL; i++)
    {
        printf("%d ",i);
        print_bytes(p->data);
        p = p->next;
    }
}
/**
 * @brief  入队(尾插)
 * @note   
 * @param  *head: 
 * @param  *tail: 
 * @retval 
 */
int add_bytes_to_queue(bytes_queue **head,bytes *data){
    
    bytes_queue *p  = *head;
    bytes_queue * tail= init_bytes_queue();
    tail->data = data;
    size_t i = 0;
    if (*head == NULL)
    {
        *head = tail;
    }else{
        
        while (p->next != NULL) //找打尾节点
        {
            p = p->next;
            i ++ ;
        }
        p->next = tail;
        
    }
    package_queue_size ++;
    return i + 1;
    
}
/**
 * @brief  从队列中出队
 * @note   
 * @param  *head: 
 * @retval 
 */
bytes *get_bytes_to_queue(){
    bytes_queue *p =package_queue;
    bytes *res = create_bytes(p->data->data,p->data->len);
    package_queue = p->next;
    p->next = NULL;
    free_bytes_queue(p);
    package_queue_size --;
    return res;
}

/**
 * @brief  释放使用的资源
 * @note   
 * @retval None
 */
void resource_release(){

    // bytes *buffer_link = NULL;
    // volatile bytes_queue *package_queue = NULL;
    // volatile int package_queue_size = 0;
    // tcp_package_info *pack_info = NULL;
    // int current_data_len; //当前检查的数据的长度
    // int base_len = 0; //底线长度 包头字节长度+数据位长度
    free_bytes(buffer_link);
    free_bytes_queue(package_queue);
    free_tcp_package_info(pack_info);
}

