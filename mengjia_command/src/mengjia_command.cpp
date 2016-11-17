#include "ros/ros.h"
#include "std_msgs/String.h"
#include <sstream>

#include<iostream>
#include "include/asoundlib.h"
#include "include/math.h"
#include "include/stdlib.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/errno.h"
#include "include/unistd.h"
#include "include/json.h"

#include "include/qisr.h"
#include "include/msp_cmn.h"
#include "include/msp_errors.h"
//#include <drv_msgs/target_info.h>

#define SAMPLE_RATE_16K     (16000)
#define MAX_GRAMMARID_LEN   (32)
#define MAX_PARAMS_LEN      (1024)
#define path1  "/home/liuxin/catkin_ws/devel/lib/mengjia_command/liuxin.wav"

const char * ASR_RES_PATH        = "fo|/home/liuxin/catkin_ws/devel/lib/mengjia_command/msc/common.jet"; //离线语法识别资源路径
const char * GRM_BUILD_PATH      = "/home/liuxin/catkin_ws/devel/lib/mengjia_command/msc/GrmBuilld"; //构建离线语法识别网络生成数据保存路径
const char * GRM_FILE            = "/home/liuxin/catkin_ws/devel/lib/mengjia_command/test.bnf"; //构建离线识别语法网络所用的语法文件

typedef struct _wave_pcm_hdr
{
    char		riff[4];
    int		size_8;
    char		wave[4];
    char		fmt[4];
    int		fmt_size;
    short int	format_tag;
    short int	channels;
    int		samples_per_sec;
    int		avg_bytes_per_sec;
    short int	block_align;
    short int	bits_per_sample;
    char 		data[4];
    int 		data_size;
} wave_pcm_hdr;



wave_pcm_hdr default_wav_hdr =
{
    { 'R', 'I', 'F', 'F' },
    0,
    {'W', 'A', 'V', 'E'},
    {'f', 'm', 't', ' '},
    16,
    1,
    1,
    16000,
    32000,
    2,
    16,
    {'d', 'a', 't', 'a'},
    0
};


typedef struct _UserData {
    int     build_fini; //标识语法构建是否完成
    int     update_fini; //标识更新词典是否完成
    int     errcode; //记录语法构建或更新词典回调错误码
    char    grammar_id[MAX_GRAMMARID_LEN]; //保存语法构建返回的语法ID
}UserData;

std::string things[10];
int numbers[10];
int number;
snd_pcm_t *handle;
int buffer_size;
snd_pcm_uframes_t frames;
void snd_pcm_open_setparams(snd_pcm_stream_t mode)
{
    int rc;
    snd_pcm_hw_params_t *params;
    unsigned int sample_rate = 16000;
    int dir;
    frames = 32;
    rc = snd_pcm_open(&handle,"default",mode,0);
    if(rc<0)
    {
        fprintf(stderr,"unable to open pcm device:%s\n",snd_strerror(rc));
        exit(1);
    }
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle,params);
    snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle,params,1);
    snd_pcm_hw_params_set_rate_near(handle,params,&sample_rate,&dir);
    snd_pcm_hw_params_set_period_size_near(handle,params,&frames,&dir);
    rc=snd_pcm_hw_params(handle,params);
    if(rc<0)
    {
        fprintf(stderr,"unable to set hw paramwters:%s\n",snd_strerror(rc));
        exit(1);
    }
    std::cout<<"参数设置成功！"<<std::endl;
    std::cout<<"============================================================="<<std::endl;
}


long convert(char d,char c)
{
    long	i;
    int	a = (int)c;
    int	b = (int)d;
    if(b>=0)
        i = a*256+b;
    else
        i = a*256+256+b;
    return i;
}




void snd_pcm_capture()
{
    FILE *fp = fopen(path1,"w");
    fwrite(&default_wav_hdr,sizeof(default_wav_hdr),1,fp);
    long o = default_wav_hdr.data_size;
    std::cout<<"请叙述你的问题："<<std::endl;
    std::cout<<"============================================================="<<std::endl;
    double amp1 = 4;
    double amp2 = 2;
    int setsilence = 400;
    int minlen = 450;
    int len=0;
    int setzcr=6;
    int silence=0;
    int status=0;
    buffer_size = frames * 2;
    char* buffer = (char*)malloc(buffer_size);
    int k;
    while(1)
    {
        snd_pcm_readi(handle,buffer,frames);
        double amp=0;
        int zcr=0;
        for(k=0;k<32;k+=2)
        {
            //amp+=pow(convert(buffer[k],buffer[k+1])/1000,2);
            amp+=(convert(buffer[k],buffer[k+1])/1000)*(convert(buffer[k],buffer[k+1])/1000);
            if(convert(buffer[k],buffer[k+1])*convert(buffer[k+2],buffer[k+3])<0 && abs(convert(buffer[k],buffer[k+1])/1000-convert(buffer[k+2],buffer[k+3])/1000)>0.02)
                zcr++;
        }
        switch(status)
        {
        case 0:
        case 1:
            if(amp>amp1)
            {
                status=2;
                silence=0;
                fwrite(buffer,1,buffer_size,fp);
                default_wav_hdr.data_size+=buffer_size;
                len++;
            }
            else if(amp>amp2||zcr>setzcr)
            {
                status=1;
                fwrite(buffer,1,buffer_size,fp);
                default_wav_hdr.data_size+=buffer_size;
                len++;
            }
            else
            {
                status=0;
                default_wav_hdr.data_size=o;
                rewind(fp);
                fwrite(&default_wav_hdr,sizeof(default_wav_hdr),1,fp);
                len=0;
            }
            break;
        case 2:
            if(amp>amp2||zcr>setzcr)
            {
                fwrite(buffer,1,buffer_size,fp);
                default_wav_hdr.data_size+=buffer_size;
                len++;
            }
            else
            {
                silence++;
                if(silence<setsilence)
                {
                    fwrite(buffer,1,buffer_size,fp);
                    default_wav_hdr.data_size+=buffer_size;
                    len++;
                }
                else if(len<minlen)
                {
                    status=0;
                    silence=0;
                    default_wav_hdr.data_size=o;
                    rewind(fp);
                    fwrite(&default_wav_hdr,sizeof(default_wav_hdr),1,fp);
                    len=0;
                }
                else
                    status=3;
            }
            break;
        case 3:
            goto ppp;
        }
    }
ppp:	std::cout<<"声音抓取成功！"<<std::endl;
    std::cout<<"============================================================="<<std::endl;
    default_wav_hdr.size_8+=default_wav_hdr.data_size+(sizeof(default_wav_hdr)-8);
    fseek(fp,4,0);
    fwrite(&default_wav_hdr.size_8,sizeof(default_wav_hdr.size_8),1,fp);
    fseek(fp, 40, 0);
    fwrite(&default_wav_hdr.data_size,sizeof(default_wav_hdr.data_size),1,fp);
    fclose(fp);
    free(buffer);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
}



int build_grammar(UserData *udata); //构建离线识别语法网络
int run_asr(UserData *udata); //进行离线语法识别



int build_grm_cb(int ecode, const char *info, void *udata)
{
    UserData *grm_data = (UserData *)udata;

    if (NULL != grm_data) {
        grm_data->build_fini = 1;
        grm_data->errcode = ecode;
    }

    if (MSP_SUCCESS == ecode && NULL != info) {
        std::cout<<"构建语法成功！ 语法ID:"<<info<<std::endl;
        if (NULL != grm_data)
            snprintf(grm_data->grammar_id, MAX_GRAMMARID_LEN - 1, info);
    }
    else
        std::cout<<"构建语法失败！"<<ecode<<std::endl;

    return 0;
}

int build_grammar(UserData *udata)
{
    FILE *grm_file                           = NULL;
    char *grm_content                        = NULL;
    unsigned int grm_cnt_len                 = 0;
    char grm_build_params[MAX_PARAMS_LEN]    = {NULL};
    int ret                                  = 0;

    grm_file = fopen(GRM_FILE, "rb");
    if(NULL == grm_file) {
        std::cout<<"打开"<<GRM_FILE<<"文件失败！"<<strerror(errno)<<std::endl;
        return -1;
    }

    fseek(grm_file, 0, SEEK_END);
    grm_cnt_len = ftell(grm_file);
    fseek(grm_file, 0, SEEK_SET);

    grm_content = (char *)malloc(grm_cnt_len + 1);
    if (NULL == grm_content)
    {
        std::cout<<"内存分配失败!"<<std::endl;
        fclose(grm_file);
        grm_file = NULL;
        return -1;
    }
    fread((void*)grm_content, 1, grm_cnt_len, grm_file);
    grm_content[grm_cnt_len] = '\0';
    fclose(grm_file);
    grm_file = NULL;

    snprintf(grm_build_params, MAX_PARAMS_LEN - 1,
             "engine_type = local, \
             asr_res_path = %s, sample_rate = %d, \
             grm_build_path = %s, ",
             ASR_RES_PATH,
             SAMPLE_RATE_16K,
             GRM_BUILD_PATH
             );
    ret = QISRBuildGrammar("bnf", grm_content, grm_cnt_len, grm_build_params, build_grm_cb, udata);

    free(grm_content);
    grm_content = NULL;

    return ret;
}


int run_asr(UserData *udata)
{
    char asr_params[MAX_PARAMS_LEN]    = {NULL};
    const char *rec_rslt               = NULL;
    const char *session_id             = NULL;
    const char *asr_audiof             = NULL;
    FILE *f_pcm                        = NULL;
    char *pcm_data                     = NULL;
    long pcm_count                     = 0;
    long pcm_size                      = 0;
    int last_audio                     = 0;
    int aud_stat                       = MSP_AUDIO_SAMPLE_CONTINUE;
    int ep_status                      = MSP_EP_LOOKING_FOR_SPEECH;
    int rec_status                     = MSP_REC_STATUS_INCOMPLETE;
    int rss_status                     = MSP_REC_STATUS_INCOMPLETE;
    int errcode                        = -1;

    asr_audiof = path1;
    f_pcm = fopen(asr_audiof, "rb");
    if (NULL == f_pcm) {
        std::cout<<"打开"<<f_pcm<<"失败！"<<strerror(errno)<<std::endl;
        goto run_error;
    }
    fseek(f_pcm, 0, SEEK_END);
    pcm_size = ftell(f_pcm);
    fseek(f_pcm, 0, SEEK_SET);
    pcm_data = (char *)malloc(pcm_size);
    if (NULL == pcm_data)
        goto run_error;
    fread((void *)pcm_data, pcm_size, 1, f_pcm);
    fclose(f_pcm);
    f_pcm = NULL;

    //离线语法识别参数设置
    snprintf(asr_params, MAX_PARAMS_LEN - 1,
             "engine_type = local, \
             asr_res_path = %s, sample_rate = %d, \
             grm_build_path = %s, local_grammar = %s, \
             result_type = json, result_encoding = UTF-8, ",
             ASR_RES_PATH,
             SAMPLE_RATE_16K,
             GRM_BUILD_PATH,
             udata->grammar_id
             );
    session_id = QISRSessionBegin(NULL, asr_params, &errcode);
    if (NULL == session_id)
        goto run_error;
    std::cout<<"开始识别......"<<std::endl;

    while (1) {
        unsigned int len = 6400;

        if (pcm_size < 12800) {
            len = pcm_size;
            last_audio = 1;
        }

        aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;

        if (0 == pcm_count)
            aud_stat = MSP_AUDIO_SAMPLE_FIRST;

        if (len <= 0)
            break;

        std::cout<<">"<<std::endl;
        fflush(stdout);
        errcode = QISRAudioWrite(session_id, (const void *)&pcm_data[pcm_count], len, aud_stat, &ep_status, &rec_status);
        if (MSP_SUCCESS != errcode)
            goto run_error;

        pcm_count += (long)len;
        pcm_size -= (long)len;

        //检测到音频结束
        if (MSP_EP_AFTER_SPEECH == ep_status)
            break;

        usleep(150 * 1000); //模拟人说话时间间隙
    }
    //主动点击音频结束
    QISRAudioWrite(session_id, (const void *)NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_status, &rec_status);

    free(pcm_data);
    pcm_data = NULL;

    //获取识别结果
    while (MSP_REC_STATUS_COMPLETE != rss_status && MSP_SUCCESS == errcode) {
        rec_rslt = QISRGetResult(session_id, &rss_status, 0, &errcode);
        usleep(150 * 1000);
    }
   std::cout<<"识别结束"<<std::endl;
    std::cout<<"============================================================="<<std::endl;
    if (NULL != rec_rslt)
    {
        std::cout<<rec_rslt<<std::endl;
        json_object *pobi = NULL;
        pobi = json_tokener_parse(rec_rslt);
        std::cout<< json_object_get_string(pobi)<<std::endl;
        pobi = json_object_object_get(pobi,"ws");
        int jslength = json_object_array_length(pobi);
        number=jslength;
        json_object *pobk[jslength];
        json_object *pobl[jslength];
        json_object *pobm[jslength];
        json_object *pobn[jslength];
        int i;
        for(i=0;i<jslength;i++)
        {
            pobk[i] = json_object_array_get_idx(pobi,i);
            std::cout<< json_object_get_string(pobk[i])<<std::endl;
            pobl[i] = json_object_object_get(pobk[i],"cw");
            std::cout<< json_object_get_string(pobl[i])<<std::endl;
            int jslength1 = json_object_array_length(pobl[i]);
            int j;
            for(j=0;j<jslength1;j++)
            {
                pobm[j] = json_object_array_get_idx(pobl[i],j);
                pobn[j] = json_object_object_get(pobm[j],"id");
                numbers[i] = json_object_get_int(pobn[j]);
                std::cout<<numbers[i]<<std::endl;
                pobm[j] = json_object_object_get(pobm[j],"w");
                things[i] =  json_object_get_string(pobm[j]);
                std::cout<<things[i]<<std::endl;
            }
        }
    }
    else
        std::cout<<"没有识别结果！"<<std::endl;
    std::cout<<"============================================================="<<std::endl;

    goto run_exit;

run_error:
    if (NULL != pcm_data) {
        free(pcm_data);
        pcm_data = NULL;
    }
    if (NULL != f_pcm) {
        fclose(f_pcm);
        f_pcm = NULL;
    }
run_exit:
    QISRSessionEnd(session_id, NULL);
    return errcode;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "mengjia_command");
    ros::NodeHandle n;
    //ros::Publisher recognize_target_pub = n.advertise<drv_msgs::target_info>("recognize/target", 20);
    const char *login_config    = "appid = 57cf7a00"; //登录参数
    UserData asr_data;
    int ret                     = 0 ;

    snd_pcm_open_setparams(SND_PCM_STREAM_CAPTURE);
    //snd_pcm_capture();

    ret = MSPLogin(NULL, NULL, login_config); //第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
    if (MSP_SUCCESS != ret) {
        std::cout<<"登录失败："<< ret<<std::endl;
        goto exit;
    }

    memset(&asr_data, 0, sizeof(UserData));
    std::cout<<"构建离线识别语法网络..."<<std::endl;
    ret = build_grammar(&asr_data);  //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建
    if (MSP_SUCCESS != ret) {
        std::cout<<"构建语法调用失败！"<<std::endl;
        goto exit;
    }
    while (1 != asr_data.build_fini)
        usleep(300 * 1000);
    if (MSP_SUCCESS != asr_data.errcode)
        goto exit;
    std::cout<<"离线识别语法网络构建完成，开始识别..."<<std::endl;
    while (ros::ok())
    {
        snd_pcm_open_setparams(SND_PCM_STREAM_CAPTURE);
        snd_pcm_capture();
        ret = run_asr(&asr_data);
        if (MSP_SUCCESS != ret) {
            std::cout<<"离线语法识别出错: "<<ret<<std::endl;
            goto exit;
        }
        //drv_msgs::target_info message1;
        for(int mim=0;mim<number;mim++)
        {
            switch (numbers[mim]) {
            case 1000:
                //message1.action.data=things[mim];
                break;
            case 100:
            {
                //message1.object.data=things[mim];
                ros::param::set("/comm/target/is_set",true);
                ros::param::set("/comm/target/label","bottle");
                break;
            }
            case 3:
                // message1.host.data=things[mim];
                break;
            case 2:
                // message1.host.data=things[mim];
                break;
            case 1:
                //message1.host.data=things[mim];
                break;
            default:
                break;
            }
        }
        //recognize_target_pub.publish(message1);
        for(int mim=0;mim<number;mim++)
        {
            std::cout<<things[mim]<<numbers[mim]<<std::endl;
        }
        std::cout<<"本次完成，下一次识别等待……"<<std::endl;
    }
exit:
    MSPLogout();
    std::cout<<"请按任意键退出..."<<std::endl;
    getchar();
    return 0;
}
