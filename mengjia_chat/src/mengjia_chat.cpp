#include "ros/ros.h"
#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "include/qtts.h"
#include "include/qisr.h"
#include "include/msp_cmn.h"
#include "include/msp_errors.h"
#include <json-c/json.h>

#define	BUFFER_SIZE	4096
#define FRAME_LEN	640
#define HINTS_SIZE  100


int ret = MSP_SUCCESS;
const char* login_params = "appid = 57cf7a00";

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
    //    rc = snd_pcm_open(&handle,"default", mode, 0);
    rc = snd_pcm_open(&handle,"default", mode, 0);
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
    printf("参数设置成功！\n");
    printf("=============================================================\n");
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
    FILE *fp = fopen("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/liuxin.wav","w");
    fwrite(&default_wav_hdr,sizeof(default_wav_hdr),1,fp);
    long o = default_wav_hdr.data_size;
    printf("请叙述你的问题：\n");
    printf("=============================================================\n");
    double amp1 = 3;
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
            amp+=pow(convert(buffer[k],buffer[k+1])/1000,2);
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
ppp:	printf("声音抓取成功！\n");
    printf("=============================================================\n");
    default_wav_hdr.size_8+=default_wav_hdr.data_size+(sizeof(default_wav_hdr)-8);
    fseek(fp,4,0);
    fwrite(&default_wav_hdr.size_8,sizeof(default_wav_hdr.size_8),1,fp);
    fseek(fp, 40, 0);
    fwrite(&default_wav_hdr.data_size,sizeof(default_wav_hdr.data_size),1,fp);
    fclose(fp);
    free(buffer);
    snd_pcm_drain(handle);
}




void run_iat(const char* audio_file)
{
    ret = MSPLogin(NULL, NULL, login_params);
    if (MSP_SUCCESS != ret)
    {
        printf("MSPLogin failed , Error code %d.\n",ret);
    }
    printf("语音语义解析开始！\n");
    printf("=============================================================\n");
    char* session_begin_params = "nlp_version =2.0,sch=1,sub=iat,domain = iat, language = utf8, accent = mandarin,aue = ico, sample_rate = 16000, result_type = plain, result_encoding = utf8";
    const char* session_id=NULL;
    char rec_result[BUFFER_SIZE];
    char hints[HINTS_SIZE];
    unsigned int total_len=0;
    int aud_stat=MSP_AUDIO_SAMPLE_CONTINUE;
    int ep_stat=MSP_EP_LOOKING_FOR_SPEECH;
    int rec_stat=MSP_REC_STATUS_SUCCESS;
    int errcode=MSP_SUCCESS;
    FILE*	f_pcm=NULL;
    char*	p_pcm=NULL;
    long	pcm_count=0;
    long	pcm_size=0;
    long	read_size=0;
    FILE*   result=fopen("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/result.txt","w");
    if (NULL == audio_file)
        goto iat_exit;
    f_pcm = fopen(audio_file, "rb");
    if (NULL == f_pcm)
    {
        printf("\nopen [%s] failed! \n", audio_file);
        goto iat_exit;
    }
    fseek(f_pcm, 0, SEEK_END);
    pcm_size = ftell(f_pcm);
    fseek(f_pcm, 0, SEEK_SET);
    p_pcm = (char *)malloc(pcm_size);
    if (NULL == p_pcm)
    {
        printf("\nout of memory! \n");
        goto iat_exit;
    }
    read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm);
    if (read_size != pcm_size)
    {
        printf("\nread [%s] error!\n", audio_file);
        goto iat_exit;
    }
    session_id = QISRSessionBegin(NULL, session_begin_params, &errcode);
    if (MSP_SUCCESS != errcode)
    {
        printf("\nQISRSessionBegin failed! error code:%d\n", errcode);
        goto iat_exit;
    }
    while (1)
    {
        unsigned int len = 10 * FRAME_LEN;
        int ret = 0;
        if (pcm_size < 2 * len)
            len = pcm_size;
        if (len <= 0)
            break;
        aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
        if (0 == pcm_count)
            aud_stat = MSP_AUDIO_SAMPLE_FIRST;
        ret = QISRAudioWrite(session_id, (const void *)&p_pcm[pcm_count], len, aud_stat, &ep_stat, &rec_stat);
        if (MSP_SUCCESS != ret)
        {
            printf("QISRAudioWrite failed! error code:%d\n", ret);
            goto iat_exit;
        }
        pcm_count += (long)len;
        pcm_size  -= (long)len;
        if (MSP_REC_STATUS_SUCCESS == rec_stat)
        {
            const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
            if (MSP_SUCCESS != errcode)
            {
                printf("QISRGetResult failed! error code: %d\n", errcode);
                goto iat_exit;
            }
            if (NULL != rslt)
            {
                unsigned int rslt_len = strlen(rslt);
                total_len += rslt_len;
                if (total_len >= BUFFER_SIZE)
                {
                    printf("no enough buffer for rec_result !\n");
                    goto iat_exit;
                }
                strncat(rec_result, rslt, rslt_len);
            }
        }
        if (MSP_EP_AFTER_SPEECH == ep_stat)
            break;
        usleep(200*1000);
    }
    errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
    if (MSP_SUCCESS != errcode)
    {
        printf("QISRAudioWrite failed! error code:%d \n", errcode);
        goto iat_exit;
    }
    while (MSP_REC_STATUS_COMPLETE != rec_stat)
    {
        const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
        if (MSP_SUCCESS != errcode)
        {
            printf("QISRGetResult failed, error code: %d\n", errcode);
            goto iat_exit;
        }
        if (NULL != rslt)
        {
            unsigned int rslt_len = strlen(rslt);
            total_len += rslt_len;
            if (total_len >= BUFFER_SIZE)
            {
                printf("no enough buffer for rec_result !\n");
                goto iat_exit;
            }
            strncat(rec_result, rslt, rslt_len);
        }
        usleep(150*1000);
    }
    printf("语音语义解析结束！\n");
    printf("=============================================================\n");
    printf("文本显示为：\n");
    printf("%s\n",rec_result);
    printf("=============================================================\n");
    fwrite(rec_result,1,BUFFER_SIZE,result);
    fclose(result);
    printf("成功将解析文本写入result.txt\n");
    printf("=============================================================\n");
iat_exit:
    if (NULL != f_pcm)
    {
        fclose(f_pcm);
        f_pcm = NULL;
    }
    if (NULL != p_pcm)
    {	free(p_pcm);
        p_pcm = NULL;
    }
    QISRSessionEnd(session_id, hints);
    MSPLogout();
}




void text_extraction()
{
    json_object	*pobj = NULL,*pobi = NULL;
    pobj = json_object_from_file("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/result.txt");
    pobi = json_object_object_get(json_object_object_get(pobj, "answer"),"text");
    if(pobi==NULL)
    {
        printf("你的问题，我无法回答！\n");
        FILE* fp=fopen("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/Mengjia.txt","w");
        fputs("你的问题，我无法回答！",fp);
        fclose(fp);
        json_object_put(pobi);
        json_object_put(pobj);
    }
    else
    {
        printf("提取的文本为：\n");
        printf("%s\n", json_object_get_string(pobi));
        json_object_to_file("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/Mengjia.txt", pobi);
        json_object_put(pobi);
        json_object_put(pobj);
    }
    printf("=============================================================\n");
}



int text_to_speech(const char* src_text, const char* des_path, const char* params)
{
    int          ret          = -1;
    FILE*        fp           = NULL;
    const char*  sessionID    = NULL;
    unsigned int audio_len    = 0;
    wave_pcm_hdr wav_hdr      = default_wav_hdr;
    int          synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
    if (NULL == src_text || NULL == des_path)
    {
        printf("params is error!\n");
        return ret;
    }
    fp = fopen(des_path, "wb");
    if (NULL == fp)
    {
        printf("open %s error.\n", des_path);
        return ret;
    }
    sessionID = QTTSSessionBegin(params, &ret);
    if (MSP_SUCCESS != ret)
    {
        printf("QTTSSessionBegin failed, error code: %d.\n", ret);
        fclose(fp);
        return ret;
    }
    ret = QTTSTextPut(sessionID, src_text, (unsigned int)strlen(src_text), NULL);
    if (MSP_SUCCESS != ret)
    {
        printf("QTTSTextPut failed, error code: %d.\n",ret);
        QTTSSessionEnd(sessionID, "TextPutError");
        fclose(fp);
        return ret;
    }
    printf("正在合成 ...\n");
    fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp);
    while (1)
    {
        const void* data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
        if (MSP_SUCCESS != ret)
            break;
        if (NULL != data)
        {
            fwrite(data, audio_len, 1, fp);
            wav_hdr.data_size += audio_len;
        }
        if (MSP_TTS_FLAG_DATA_END == synth_status)
            break;
        usleep(150*1000);
    }
    if (MSP_SUCCESS != ret)
    {
        printf("QTTSAudioGet failed, error code: %d.\n",ret);
        QTTSSessionEnd(sessionID, "AudioGetError");
        fclose(fp);
        return ret;
    }
    wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);
    fseek(fp, 4, 0);
    fwrite(&wav_hdr.size_8,sizeof(wav_hdr.size_8), 1, fp);
    fseek(fp, 40, 0);
    fwrite(&wav_hdr.data_size,sizeof(wav_hdr.data_size), 1, fp);
    fclose(fp);
    fp = NULL;
    ret = QTTSSessionEnd(sessionID, "Normal");
    if (MSP_SUCCESS != ret)
    {
        printf("QTTSSessionEnd failed, error code: %d.\n",ret);
    }
    return ret;
}



void run_tts()
{
    FILE* fp=fopen("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/Mengjia.txt","r");
    fseek(fp,0,SEEK_END);
    long length=ftell(fp);
    fseek(fp,0,SEEK_SET);
    char* text=(char*)malloc(length);
    fread(text,1,length,fp);
    text[length-1]='\0';
    fclose(fp);
    ret = MSPLogin(NULL, NULL, login_params);
    if (MSP_SUCCESS != ret)
    {
        printf("MSPLogin failed , Error code %d.\n",ret);
    }
    printf("开始合成 ...\n");
    printf("=============================================================\n");
    char* session_begin_params = "voice_name = xiaoyan, text_encoding = utf8, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2";
    char* filename= "/home/liuxin/catkin_ws/devel/lib/mengjia_chat/Mengjia.wav";
    ret = text_to_speech(text, filename, session_begin_params);
    if (MSP_SUCCESS != ret)
    {
        printf("text_to_speech failed, error code: %d.\n", ret);
    }
    free(text);
    MSPLogout();
    printf("合成完毕\n");
    printf("=============================================================\n");
}


void snd_pcm_playback()
{
    char* buffer = (char*)malloc(buffer_size);
    printf("开始回答您的问题：\n");
    printf("=============================================================\n");
    FILE* fp=fopen("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/Mengjia.wav","rb");
    while(!feof(fp))
    {
        fread(buffer,1,buffer_size,fp);
        snd_pcm_writei(handle,buffer,frames);
    }
    fclose(fp);
    free(buffer);
    snd_pcm_drain(handle);
    printf("回答完毕！\n");
    printf("=============================================================\n");
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "mengjia_chat");
    ros::NodeHandle n;
    while(ros::ok())
    {
        snd_pcm_open_setparams(SND_PCM_STREAM_CAPTURE);
        snd_pcm_capture();
        snd_pcm_close(handle);
        run_iat("/home/liuxin/catkin_ws/devel/lib/mengjia_chat/liuxin.wav");
        text_extraction();
        run_tts();
        snd_pcm_open_setparams(SND_PCM_STREAM_PLAYBACK);
        snd_pcm_playback();
        snd_pcm_close(handle);
    }
    return 0;
}




