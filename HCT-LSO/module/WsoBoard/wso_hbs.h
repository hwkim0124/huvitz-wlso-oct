/*
 * hbs.h
 *
 *  Created on: 2019. 10. 20.
 *
 */

#ifndef SRC_SLO_HBS_H_
#define SRC_SLO_HBS_H_

#include "wso_ctypes.h"



namespace wso_board
{
#define HBS_MAX_TBL_ITEM_NUM 48
#define HBS_TBL_ITEM_NUM 31 // ACTIVE Table ITEM
#define BLK_BUF_TBL_NUM 4

#define TBL_BLKBUF_ID 0
#define TBL_CALBLKBUF_ID 1
#define TBL_SYSCAL_ID 2         // Reserved. 
#define TBL_MB_VER_INFO_ID 3
#define TBL_SYS_INIT_STATUS_ID 4
#define TBL_SYS_CFG_ID 5
#define TBL_GP_STATUS_ID 6
#define TBL_SLD_STATUS_ID 7
#define TBL_ZYNQ_XADC_ID 8
#define TBL_OCT_FOCUS_MOTOR_ID 9
#define TBL_OCT_REF_MOTOR_ID 10
#define TBL_OCT_POL_MOTOR_ID 11
#define TBL_REF_ND_MOTOR_ID 12
#define TBL_LSO_FOCUS_MOTOR_ID 13
#define TBL_RM_MOTOR_ID 14
#define TBL_ANTLENS_MOTOR_ID 15
#define TBL_AP1_MOTOR_ID 16
#define TBL_AP2_MOTOR_ID 17
#define TBL_SWING_MOTOR_ID 18
#define TBL_X_MOTOR_ID 19
#define TBL_Z_MOTOR_ID 20
#define TBL_Y_MOTOR_ID 21
#define TBL_CR_MOTOR_ID 22
#define TBL_LED_STATUS_ID 23
#define TBL_LSO_SCANNER_ID 24
#define TBL_OCT_GAVANO_ID 25
#define TBL_RET_IR_CAM_STATUS_ID 26
#define TBL_ANT_IR_CAM_0_STATUS_ID 27   // Low Side
#define TBL_ANT_IR_CAM_1_STATUS_ID 28   // Left
#define TBL_ANT_IR_CAM_2_STATUS_ID 29   // Right

#define HBS_TBL_VERSION 1
#define HBS_TBL_DESCRIPTOR_ADDR 0xFFFF0000

#define MAX_LSO_DAC_SAMPLE_SIZE 8192
#define MAX_OCT_DAC_SAMPLE_SIZE 16384
#define MAX_TRAJ_NUM 32
#define MAX_LSO_TRAJ_NUM 8
#define MAX_STEP_MOTOR_NUM 16

#define CALIB_IDX_MOTOR_SETS            0
#define CALIB_IDX_OCT_PARAMS            1
#define CALIB_IDX_LED_SOURCE	        2
#define CALIB_IDX_OCT_GALVANO	        3   
#define CALIB_IDX_DEVICE_CFG	        4
#define CALIB_IDX_STEP_MOTORS	        5
#define CALIB_IDX_FACTORY_SET1	        8
#define CALIB_IDX_FACTORY_SET2   	    9
#define CALIB_ENTRIES_MAX_NUM			16

#define BUFF_ADDR_OFFS_MOTOR_SETS       0x00000
#define BUFF_ADDR_OFFS_OCT_PARAMS       0x10000
#define BUFF_ADDR_OFFS_OCT_SOURCE       0x20000
#define BUFF_ADDR_OFFS_OCT_GALVANO      0x30000
#define BUFF_ADDR_OFFS_DEVICE_CFG       0x40000
#define BUFF_ADDR_OFFS_STEP_MOTORS      0x50000
#define BUFF_ADDR_OFFS_FACTORY_SET1     0x80000
#define BUFF_ADDR_OFFS_FACTORY_SET2     0x90000

#define CALIB_ACTIVE_REGION             0 
#define CALIB_BACKUP_REGION             1

#pragma pack(push, 4)

    /////////////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        U32 ver;
        U32 count;
        U32 header_chksum;
        U32 table_chksum;
    } hbs_table_header_st;

    typedef struct {
        U32 buf_addr;
        U32 buf_size;
    } hbs_table_entry_st;

    typedef struct {
        hbs_table_header_st header;
        hbs_table_entry_st entries[HBS_MAX_TBL_ITEM_NUM];
    } hbs_table_descriptor_st;
    
    typedef struct {
        U32 count;
        U32 header_chksum;
        U32 table_chksum;
    } bulk_buffer_header_st;

    typedef struct {
        U32 buf_addr;
        U32 buf_size;
    } bulk_buffer_entry_st;

    typedef struct {
        bulk_buffer_header_st header;
        bulk_buffer_entry_st entries[BLK_BUF_TBL_NUM];
    } bulk_buffer_descriptor_st;



    //////////////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        S8 app_ver[8];  // Applicatin version
        S8 fpga_ver[8]; // FPGA version
    } MainBoardVerInfo_st;

    typedef struct {
        U32 status; // 0 initializing,1: fail,2 OK
        U32 dev_status;
        U16 DevErrCode[32];
        U16 model_cfg; // model id added , 2018.11.21
    } SysInitStatus_st;

    typedef struct {
        S8 SysSerialNo[16];
        S8 ProcessNo[16];
        S8 SLD_SN[8];
    } SysMfgInfo_st;

    typedef struct // 64bytes
    {
        U16 UartDebugEn;
        U16 SLDWarningEn;
        U16 StepMotorDiag;
        U16 AutoSlew;
        U32 SlewRate;
    } SysCfg_st; // 256 bytes


    //////////////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        S16 InPos;
        S16 OutPos;
    } InOutPos_st;
    
    typedef struct {
        U32 crc;
        S32 REF_RetinaPos;
        S32 REF_CorneaPos;
        S32 PolarizationPos;
        S32 lso_focus_motor_0D_pos;
        S32 oct_focus_motor_0D_pos;
        InOutPos_st ReturnMirrorPos;
        InOutPos_st OCT_AntLensPos; // OCT Anterior Lens position.
    } MotorCalPos_st;

    typedef struct {
        S32 CurPos;
        S32 PI_status; // bit0: LOW PI hit, bit1: high PI hit
        U32 max_speed;
        U32 min_speed;
        U32 acc_step;
        S32 sm_pos_min; // minus end position,added 2013.9.27;
        S32 sm_pos_max; // plus end position//added 2013.9.27;

        // variables for PI position monitoring.
        U32 MotorWait;           // 2019.11.07
        S32 sm_pi_hit_ref_pos;   // expected PI signal level change position
        S32 sm_pi_hit_margin;    // expected PI signal level margin, ref_pos+- margin.
        S32 sm_last_pi_hit_pos;  // last PI signal change position;
        S32 sm_pi_hit_pos_error; // set to 1 when pi hit position is outside of ref
        // pos+- margin, cleared when system initialized.
    } StepMotorInfo_st;

    typedef struct {
		S16 LimitSensorStatus[2]; // [0] : Low limit, [1] : High limit
        S16 limit_overide;        // 2019.11
    } DC_CR_Motor_st;

    typedef struct {
        U32 min_speed_pps;
        U32 max_speed_pps;
        U32 acc_step;

        S32 sm_init_dir;
        S32 sm_pos_min; // minus end position,added 2013.9.27;
        S32 sm_pos_max; // plus end position//added 2013.9.27;
        S32 sm_init_pos;
        S32 pi_detect_max;
        S32 sm_pi_escape_pulse;
        
        S32 sm_pi_hit_ref_pos;   // expected PI signal level change position
        S32 sm_pi_hit_margin;    // expected PI signal level margin, ref_pos+- margin.
    } step_m_init_table_st;


    ///////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        U16 IM_MAX;         // uA, SLD interal PD current at MAX cornea power
        U16 IM_MIN;         // uA, SLD internal PD current at Min Cornea power
        U16 IS_MAX;         // mA, SLD current at Max cornea power
        U16 IS_MIN;         // mA, SLD current at Min Cornea power
        U16 DN_REF_EPD_MAX; // MAX external REF PD level in Digit number
        U16 DN_REF_EPD_MIN; // MIN external REF PD level in Digit number

        U16 RmonHighCode; // ISL22317 Digital Poentiometer code
        U16 RmonLowCode1; // AD5252 Digital Poentiometer code
        U16 RmonLowCode2; // AD5252 Digital Poentiometer code
        U16 RsiCode;      // ISL22317 Digital Poentiometer code
    } SLDparam_st;

    typedef struct {
        float SLD_current; // mA
        float IPD_current; // uA
        U32 EPD_DN;        // digit nunmber
        float temp;        // celsius
    } SLD_status_st;

    typedef struct {
        SLDparam_st SLD_Param;
        SLD_status_st SLD_status; //
    } SLD_st;

    typedef struct {
        S32 PktErrCode;
        U32 Intr_src;
        U32 PIstatus;
        U32 UpgradeStatus;
        U32 UpgradeProgressCnt;
    } GP_Status_st; //

    typedef struct {
        float cpu_temp;    // cpu temperature.
        U32 ext_ld_mon_pd; // external LD monitoring PD sensor data.
        U32 Hall_Sensor1;  // adapter lens hall sensor1
        U32 Hall_sensor2;  // adapter lens hall sensor2
        U32 odos;
    } Zynq_XADC_st;

    typedef struct {
        U32 reserved[4];
    } IRCamInfo_st;

    typedef struct {
        U16 WLED_intensity;
        U16 RetIR_intensity;
        U16 AntIR1_intensity;
        U16 AntIR2_intensity;
        U16 Bled_intensity;
        U16 Gled_inentity;
    } LED_Info_st;


    //////////////////////////////////////////////////////////////////////////////////////
    typedef struct
    {
        float time_step_us;				//galvano scanner control sample time.
        float exposure_time_us;			//camera exposure time in us.

        U16 TrgSrc;						//camera flash out source 0: ColorCamera,1:IR camera. 
        U16 AcqMode;					//0:continuous, 1:fixed frame Mode.
        U32 AcqFrameSize;				//valid on Fixed FrameMode

        U16 sample_size;				//galvano control pattern sample size
        U16 subframe_size;				//subframe size of a Full frame.
        S16 prescan_pos_rewind_offset;	//gavano rewind offset of subframe
        U16 led_on_pos_index;			//led on sample index of gavano pattern.
        U16 led_off_pos_index;			//led off sample index of galvano pattern.
        S16 pos[MAX_LSO_DAC_SAMPLE_SIZE]; //scanning pattern.
    } lso_scan_profile_st;

    typedef struct {
        U32 ctrl_status; // on,off
        U32 acq_cnt;     // current acq.cnt
    } lso_capture_status_st;

    typedef struct {
        lso_capture_status_st lso_capture_status;
        lso_scan_profile_st traj_profile[MAX_LSO_TRAJ_NUM + 1];
    } LsoScanner_st;

    
    //////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        U32 crc;
        U32 data_size; // data size of bytes of the  effective data from crc(head
        // data) field to the active end of UserCalblk_st, M/B will
        // load data_size size at initialization time from flash.Max
        // 64kbytes.
        U32 reserved[(0x10000 / 4) - 8]; // 64kbytes-8bytes ,reserved flash space(user
        // defined data format)
    } FactorySetupCalBlk_st;           // MAX 64kbytes Block size.

    typedef struct {
        float Galvano_offset;
        float Galvano_Range;
    } GalvanoCal_st;

    typedef struct {
        double a0;
        double a1;
        double a2;
        double a3;
    } SpectroCal_st;


    typedef struct {
        double a2; // a2
        double a3; // a3
        double a4; // a4
    } Dispersion_st;

    typedef struct
    {
        S32 oct_focus_zeroD_pos;
        S32 Scan_focus_zeroD_pos;
        S32 fixation_focus_zeroD_pos;
    } DiopterCal_st; //


    //////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        float time_step_us;
        U32 trig_delay; // 1 cnt: 1HCLK period
        U16 repeat_num;
        U16 sample_size;
        U16 cam_trig_cnt;
        U16 cam_trig_itv;  // camera trigger interval,: # of position samples
        U16 trig_st_index; // trigger start index
        U16 reserved[23];  // reserved
        S16 posx[MAX_OCT_DAC_SAMPLE_SIZE]; // place the array end of struct for the
        // variable size of sample size
        S16 posy[MAX_OCT_DAC_SAMPLE_SIZE];
    } traj_profile_st;

    typedef struct {
        S16 offsetX;
        S16 offsetY;
    } Galvano_Dynamic_st;

    typedef struct {
        traj_profile_st traj_profile1[MAX_TRAJ_NUM + 1];
        Galvano_Dynamic_st Galvano_Dynamic;
    } OctGalvano_st;

    typedef struct {
        S16 Base_phase;
        S16 Phase_inc;
        U16 start_phase_trig_index; // valid only phase_trig_src is 0
        U16 end_phase_trig_index;   // valid only phase_trig_src is 0
        U32 trig_delay_cnt;         // 1:10ns

        U32 end_phase_wait_time_us;    //
        U16 phase_origin_slew_time_us; //
        U16 phase_inc_count; // counts of phase increment steps at Phase ramp period ,

        U8 reserved1;
        U8 reserved2;
        U16 reserved3;
    } OctPhaseCtrl_st;

    //////////////////////////////////////////////////////////////////////////////////////
    typedef struct {
        U32 crc;
        U32 RomBlkBaseAddr; // Flash ROM each CalBlkx start addr
        U32 RomBlkSize; // Flash ROM each Calblkx size of bytes : sizeof(CalBlkx_st)
        // bytes.
        U32 HBS_BlkBaseAddr; // address of HBS Calblkx M/B RAM structure address. use
        // as data update address
    } CalBlkInfo_st;

    typedef struct {
        U32 CalBlkBitMap; // #of Active calibratin blockBitMap. If blk0,1 used, set as
        // 0x3,                          //0~65535 decimal If all 16
        // blks used, 0xffff
        CalBlkInfo_st blocks[16]; // CalBlkInfo[0] corresponds to CalkBlk0, ��
    } HBS_CalBlockBuffer_st; // RAM memory layout for calibration data interface.

    typedef struct {
        U32 crc; //
        MotorCalPos_st MotorCalPos;
    } CalBlk0_st;

    typedef struct {
        U32 crc; //
        Dispersion_st RetinaDispersion;
        Dispersion_st CorneaDispersion;
        SpectroCal_st SpectroCal;
    } CalBlk1_st; // OCT system calibration parmeters

    typedef struct {
        U32 crc; //
        SLDparam_st SLD_Param;
        LED_Info_st LED_Info;
    } CalBlk2_st; // light source calibration parameters

    typedef struct {
        U32 crc; //
        GalvanoCal_st OctGalvano_Xcal;
        GalvanoCal_st OctGalvano_Ycal;
    } CalBlk3_st; // galvano calibration parameters.reserved.

    typedef struct {
        char DeviceSN[32];     // Max 32 bytes, ASCII String
        char MfgDate[16];      // Max 16 bytes,ASCII string.
        char DeviceProcNo[32]; // Max 32 bytes, ASCII string
    } MfgDeviceRecord_st;    // Manufacturing Device Recode

    typedef struct {
        U8 UseModuleEnables;     // 1:enable, 0:disable
        U8 MotorEnable[16];      // If 1:enabled, 0:disabled.
        U8 ModuleEnableMask[16]; // If 1:enabled, 0:disabled.
    } ModuleEnable_st;

    typedef struct {
        U32 crc; //
        MfgDeviceRecord_st MfgDeviceRecord;
        ModuleEnable_st ModuleEnable;
        U16 model_cfg;//Combo:0x0,OCT:0x10,Fundus:0x20,Fundus_FA:0x21,Fundus_ICG:0x22
    } CalBlk4_st; // galvano calibration parameters.reserved.

    typedef struct {
        U32 crc;
        step_m_init_table_st step_m_init_table[MAX_STEP_MOTOR_NUM];
        U16 sm_dac_ch_low_torque[MAX_STEP_MOTOR_NUM];
        U16 sm_dac_ch_high_torque[MAX_STEP_MOTOR_NUM];
    } CalBlk5_st;

    typedef struct {
        U32 crc;
        U32 data_size; // data size of bytes of the  effective data from crc(head
        // data) field to the active end of UserCalblk_st, M/B will
        // load data_size size at initialization time from flash.Max
        // 64kbytes.
        U32 reserved[(0x10000 / 4) - 8]; // 64kbytes-8bytes ,reserved flash space(user
        // defined data format)
    } CalBlk6_st; // MAX 64kbytes Block size.

    typedef struct {
        U32 crc;
        U32 data_size; // data size of bytes of the  effective data from crc(head
        // data) field to the active end of UserCalblk_st, M/B will
        // load data_size size at initialization time from flash.Max
        // 64kbytes.
        U32 reserved[(0x10000 / 4) - 8]; // 64kbytes-8bytes ,reserved flash space(user
        // defined data format)
    } CalBlk7_st; // MAX 64kbytes Block size.

    //////////////////////////////////////////////////////////////////////////////////////
    typedef struct
    {
        /*
        S32 REF_RetinaPos;
        S32 REF_CorneaPos;
        S32 PolarizationPos;
        Dispersion_st   RetinaDispersion;
        Dispersion_st   CorneaDispersion;
        SpectroCal_st   SpectroCal;
        GalvanoCal_st   OctGalvano_Xcal;
        GalvanoCal_st   OctGalvano_Ycal;
        SLDparam_st     SLD_Param;
        // DiopterCal_st   DiopterCal; //focus motors calibration
        float			slo_adc_offset[3];
        float 			slo_det_gain_ratio[3];//0.000 ~1.000
        U64 chksum;//8 bytes.
        */
        S32 REF_RetinaPos;
        S32 REF_CorneaPos;
        S32 PolarizationPos;
        Dispersion_st   RetinaDispersion;
        Dispersion_st   CorneaDispersion;
        SpectroCal_st   SpectroCal;
        GalvanoCal_st   OctGalvano_Xcal;
        GalvanoCal_st   OctGalvano_Ycal;

        LED_Info_st		LED_Cal;
        DiopterCal_st   DiopterCal; //focus motors calibration
        U64 chksum;//8 bytes.
    } SysCal_st;

    typedef struct
    {
        /*
        slo_capture_mode_st slo_capture_modes[8]; // 8 x 48 = 384 bytes
        slo_imaging_mode_st slo_imaging_modes[6]; // 6 x 64 = 384 bytes
        slo_capture_init_st slo_capture_init; // 32 bytes
        fixation_setup_st fixation_setup; // 32 bytes
        fixation_preset_st fixation_preset[2]; // 36 x 2 = 72 bytes
        cornea_camera_st cornea_camera; // 8 bytes
        working_dot_st wdots[2]; // 4 x 2 = 8 bytes  
        oct_galvano_st oct_galvano; // 64 bytes
        slo_capture_mode_st slo_capture_wides[8]; // 8 x 48 = 384 bytes
        slo_imaging_mode_st slo_imaging_wides[6]; // 6 x 64 = 384 bytes
        */
        U64 chksum; // 8 bytes. => 928 bytes
        U8 reserved[6424];
    } UserSetup_st;//8192=8kbytes max

#pragma pack(pop)
}

#endif /* SRC_BIO_HBS_H_ */
