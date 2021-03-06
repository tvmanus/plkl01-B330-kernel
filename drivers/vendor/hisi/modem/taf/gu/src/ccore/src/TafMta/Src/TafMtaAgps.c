

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "TafMtaComm.h"
#include "TafMtaMain.h"
#include "AtMtaInterface.h"
#include "TafMtaTimerMgmt.h"
#include "TafMtaAgps.h"
#include "TafAppMma.h"
#include "MnComm.h"

#include "NasUtranCtrlInterface.h"
#include "TafSdcLib.h"


/*lint -e958*/

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
/*lint -e767 */
#define    THIS_FILE_ID                 PS_FILE_ID_TAF_MTA_AGPS_C
/*lint +e767 */

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if (FEATURE_ON == FEATURE_AGPS)


VOS_UINT32 TAF_MTA_SndAtXcposrRptInd(VOS_UINT32 ulClearFlg)
{
    AT_APPCTRL_STRU                     stAppCtrl;                    /* 接收消息指针 */
    MTA_AT_XCPOSRRPT_IND_STRU           stXcposrRptInd;

    /* 填写消息结构 */
    stXcposrRptInd.ulClearFlg = ulClearFlg;

    /* 填写事件客户端类型为广播事件 */
    stAppCtrl.usClientId      = MTA_CLIENTID_BROADCAST;
    stAppCtrl.ucOpId          = MTA_INVALID_TAB_INDEX;

    /* 给AT模块发送ID_MTA_AT_XCPOSRRPT_IND消息 */
    TAF_MTA_SndAtMsg( &stAppCtrl,
                      ID_MTA_AT_XCPOSRRPT_IND,
                      sizeof(MTA_AT_XCPOSRRPT_IND_STRU),
                      (VOS_UINT8*)&stXcposrRptInd );

    return VOS_OK;
}


VOS_UINT32 TAF_MTA_SndAtCposrInd(
    XML_COMM_NODE_STRU                 *pstNode,
    XML_COMM_CTX_STRU                  *pstXmlCommCtx
)
{
    VOS_CHAR                           *pcXmlStr;                               /* XML码流首地址 */
    VOS_CHAR                           *pcXmlStrCur;                            /* XML码流当前位置 */
    VOS_UINT32                          i;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulXmlLen;
    AT_APPCTRL_STRU                     stAppCtrl;                              /* 接收消息指针 */
    MTA_AT_CPOSR_IND_STRU              *pstCposrInd;
    XML_COMM_RESULT_ENUM_UINT32         enResult;

    /* 内存申请、初始化 */
    pstCposrInd = PS_MEM_ALLOC(UEPS_PID_MTA, sizeof(MTA_AT_CPOSR_IND_STRU));
    if (VOS_NULL_PTR == pstCposrInd)
    {
        MTA_ERROR_LOG("TAF_MTA_SndCposrInd: Message malloc Failed!");
        return VOS_ERR;
    }
    PS_MEM_SET(pstCposrInd, 0x00, sizeof(MTA_AT_CPOSR_IND_STRU));

    /* XML码流动态存储申请、初始化 */
    pcXmlStr = PS_MEM_ALLOC( UEPS_PID_MTA, MTA_XML_TEXT_MAX_BUFF_SIZE);
    if (VOS_NULL_PTR == pcXmlStr)
    {
        PS_MEM_FREE(UEPS_PID_MTA, pstCposrInd);
        MTA_ERROR_LOG("TAF_MTA_SndCposrInd: Xml text malloc Failed!");
        return VOS_ERR;
    }
    PS_MEM_SET(pcXmlStr, 0x00, MTA_XML_TEXT_MAX_BUFF_SIZE);

    /* 构造XML码流, 若码流超长，则返回VOS_ERR*/
    ulXmlLen = MTA_XML_TEXT_MAX_BUFF_SIZE;
    enResult =  XML_COMM_BuildXMLStr(pstNode, pcXmlStr, &ulXmlLen, XML_FORMATTING_NONE);

    /* 如果处理失败或者XML码流超过最大长度，则释放内存后退出 */
    if (XML_COMM_RESULT_SUCCEED != enResult)
    {
        PS_MEM_FREE(UEPS_PID_MTA, pcXmlStr);
        PS_MEM_FREE(UEPS_PID_MTA, pstCposrInd);
        MTA_WARNING_LOG("TAF_MTA_SndCposrInd: Build XML text Failed!");
        return VOS_ERR;
    }

    /* 填写上报类型为广播事件 */
    stAppCtrl.usClientId = MTA_CLIENTID_BROADCAST;
    stAppCtrl.ucOpId     = MTA_INVALID_TAB_INDEX;

    /* 填写消息结构 */
    pcXmlStrCur          = pcXmlStr;

    /* XML码流分割后发送 */
    ulLoop = (VOS_UINT32)(ulXmlLen/(MTA_CPOSR_XML_MAX_LEN));

    /* 每次发送1024个字节的码流 + '\0' */
    for (i = 0; i < ulLoop; i++)
    {
        /* 拷贝前1024个字符，码流末尾赋为'\0' */
        PS_MEM_CPY(pstCposrInd->acXmlText, pcXmlStrCur, MTA_CPOSR_XML_MAX_LEN);
        pstCposrInd->acXmlText[MTA_CPOSR_XML_MAX_LEN] = '\0';
        pcXmlStrCur                                  += MTA_CPOSR_XML_MAX_LEN;

        /* 给AT模块发送ID_MTA_AT_CPOSR_IND消息 */
        TAF_MTA_SndAtMsg( &stAppCtrl,
                          ID_MTA_AT_CPOSR_IND,
                          sizeof(MTA_AT_CPOSR_IND_STRU),
                          (VOS_UINT8*)pstCposrInd);

    }

    /* 再发送剩下的码流 + '\0' */
    PS_MEM_SET(pstCposrInd, 0, sizeof(MTA_AT_CPOSR_IND_STRU));
    PS_MEM_CPY(pstCposrInd->acXmlText, pcXmlStrCur, (VOS_UINT32)(ulXmlLen - (VOS_UINT32)(pcXmlStrCur - pcXmlStr)));
    pstCposrInd->acXmlText[(VOS_UINT32)(ulXmlLen - (VOS_UINT32)(pcXmlStrCur - pcXmlStr))] = '\0';

    /* 给AT模块发送ID_MTA_AT_CPOSR_IND消息 */
    TAF_MTA_SndAtMsg( &stAppCtrl,
                      ID_MTA_AT_CPOSR_IND,
                      sizeof(MTA_AT_CPOSR_IND_STRU),
                      (VOS_UINT8*)pstCposrInd);

    /* 释放申请的内存 */
    PS_MEM_FREE(UEPS_PID_MTA, pstCposrInd);
    PS_MEM_FREE(UEPS_PID_MTA, pcXmlStr);

    return VOS_OK;
}


VOS_UINT32 TAF_MTA_ProcAssistData(AGPS_ASSIST_DATA_STRU *pstAssistData)
{
    XML_COMM_CTX_STRU                   stXmlCommCtx;                           /* XML树存储结构 */
    XML_COMM_NODE_STRU                 *pstNodePos;                             /* XML根节点 */
    XML_COMM_NODE_STRU                 *pstNodeAssistData;
    VOS_UINT32                          ucResult;
    XML_COMM_RESULT_ENUM_UINT32         enXmlRslt;

    /* 局部变量初始化 */
    PS_MEM_SET(&stXmlCommCtx, 0x00, sizeof(XML_COMM_CTX_STRU));

    /* 初始化XmlCtx */
    if (XML_COMM_RESULT_SUCCEED != XML_COMM_InitXMLCtx(&stXmlCommCtx))
    {
        XML_COMM_ClearXMLCtx(&stXmlCommCtx);
        MTA_WARNING_LOG("TAF_MTA_ProcAssistData: WARNING: Init xmlCtx fail!");
        return VOS_ERR;
    }

    /* 调用XML树结构构造函数,构造XML结构树 */
    pstNodePos = TAF_MTA_AGPS_ConvertPos(&stXmlCommCtx);
    if (VOS_NULL_PTR == pstNodePos)
    {
        XML_COMM_ClearXMLCtx(&stXmlCommCtx);
        MTA_WARNING_LOG("TAF_MTA_ProcAssistData: WARNING: Create pos fail!");
        return VOS_ERR;
    }

    /* 构造Assist Data数据结构 */
    pstNodeAssistData = TAF_MTA_AGPS_ConvertAssistData(&stXmlCommCtx, pstAssistData);
    if (VOS_NULL_PTR == pstNodeAssistData)
    {
        XML_COMM_ClearXMLCtx(&stXmlCommCtx);
        MTA_WARNING_LOG("TAF_MTA_ProcAssistData: WARNING: Create assist data fail!");
        return VOS_ERR;
    }

    /* 构造完整的POS数结构 */
    enXmlRslt = XML_COMM_AddAChildNode(pstNodePos, pstNodeAssistData, VOS_NULL_PTR);
    if (XML_COMM_RESULT_SUCCEED != enXmlRslt)
    {
        XML_WARNING_LOG("XML_COMM_AddAChildNode: Error!");
    }

    /* XML树转化为XML码流，发送到AT模块 */
    ucResult = TAF_MTA_SndAtCposrInd(pstNodePos, &stXmlCommCtx);

    /* 释放XMLCTX资源 */
    XML_COMM_ClearXMLCtx(&stXmlCommCtx);

    return ucResult;
}
VOS_VOID TAF_MTA_GetRemainDgpsCorrectionsNodeNum(
    AGPS_GPS_ASSIST_STRU               *pstGpsAssist,
    VOS_UINT32                         *pulRemainDgpsCorrectionNum
)
{
    if (VOS_TRUE == pstGpsAssist->bitOpDgpsCorrection)
    {
        if (MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET <= pstGpsAssist->stDgpsCorrection.ulDgpsCorrectionNum)
        {
            *pulRemainDgpsCorrectionNum = pstGpsAssist->stDgpsCorrection.ulDgpsCorrectionNum
                                          - MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET;
        }
        else
        {
            *pulRemainDgpsCorrectionNum = 0;
        }
    }
    else
    {
        *pulRemainDgpsCorrectionNum = 0;
    }

    return;
}


VOS_VOID TAF_MTA_GetRemainGpsRtIntegrityNodeNum(
    AGPS_GPS_ASSIST_STRU               *pstGpsAssist,
    VOS_UINT32                         *pulRemainBadSatListBadSatListNum
)
{
    if (VOS_TRUE == pstGpsAssist->bitOpGpsRealTimeIntegrity)
    {
        if (MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET <= pstGpsAssist->stGpsRealTimeIntegrity.ulBadSatListBadSatListNum)
        {
            *pulRemainBadSatListBadSatListNum = pstGpsAssist->stGpsRealTimeIntegrity.ulBadSatListBadSatListNum
                                                - MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET;
        }
        else
        {
            *pulRemainBadSatListBadSatListNum = 0;
        }
    }
    else
    {
        *pulRemainBadSatListBadSatListNum = 0;
    }

    return;
}


VOS_UINT32 TAF_MTA_ProcExtraAssistData(AGPS_ASSIST_DATA_STRU *pstAssistData)
{
    AGPS_GPS_ASSIST_STRU               *pstGpsAssist;
    AGPS_ASSIST_DATA_STRU              *pstTmpAssistData;
    VOS_UINT32                          ulRemainDgpsCorrectionNum;
    VOS_UINT32                          ulRemainBadSatListBadSatListNum;
    VOS_UINT32                          ulFirstLoop;
    VOS_UINT32                          ulSecondLoop;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulResult;

    pstGpsAssist = &pstAssistData->stGpsAssist;

    /* 获取剩余的<DGPS_corrections>节点个数 */
    TAF_MTA_GetRemainDgpsCorrectionsNodeNum(pstGpsAssist, &ulRemainDgpsCorrectionNum);

    /* 获取剩余的<GPS_rt_integrity>节点个数 */
    TAF_MTA_GetRemainGpsRtIntegrityNodeNum(pstGpsAssist, &ulRemainBadSatListBadSatListNum);

    /* 没有剩余的<DGPS_corrections>节点或者<GPS_rt_integrity>节点，返回不处理 */
    if ((0 == ulRemainDgpsCorrectionNum)
     && (0 == ulRemainBadSatListBadSatListNum))
    {
        return VOS_OK;
    }

    /* 剩余的<DGPS_corrections>节点个数大于15，按15处理 */
    if (ulRemainDgpsCorrectionNum > (AGPS_MAX_SAT_NUM - MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET))
    {
        ulRemainDgpsCorrectionNum = AGPS_MAX_SAT_NUM - MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET;
    }

    /* 剩余的<GPS_rt_integrity>节点个数大于15，按15处理 */
    if (ulRemainBadSatListBadSatListNum > (AGPS_MAX_SAT_NUM - MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET) )
    {
        ulRemainBadSatListBadSatListNum = AGPS_MAX_SAT_NUM - MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET;
    }

    /* 计算第一轮循环和第二轮循环的循环次数 */
    if (ulRemainDgpsCorrectionNum >= ulRemainBadSatListBadSatListNum)
    {
        ulFirstLoop  = ulRemainBadSatListBadSatListNum;
        ulSecondLoop = ulRemainDgpsCorrectionNum - ulRemainBadSatListBadSatListNum;
    }
    else
    {
        ulFirstLoop  = ulRemainDgpsCorrectionNum;
        ulSecondLoop = ulRemainBadSatListBadSatListNum - ulRemainDgpsCorrectionNum;
    }

    /* 申请动态内存，用于缓存构造的pstTmpGpsAssist结构体 */
    pstTmpAssistData = (AGPS_ASSIST_DATA_STRU *)PS_MEM_ALLOC(UEPS_PID_MTA, sizeof(AGPS_ASSIST_DATA_STRU));
    if (VOS_NULL_PTR == pstTmpAssistData)
    {
        MTA_ERROR_LOG("TAF_MTA_ProcExtraAssistData: ERROR: Alloc mem fail!");
        return VOS_ERR;
    }

    PS_MEM_SET(pstTmpAssistData, 0x00, sizeof(AGPS_ASSIST_DATA_STRU));

    /* 第一轮循环 */
    for (ulLoop = 0; ulLoop < ulFirstLoop; ulLoop++)
    {
        pstTmpAssistData->bitOpGpsAssist = VOS_TRUE;
        /* 先将bit位全清为0后，再设置<DGPS_corrections>和<GPS_rt_integrity>节点的bit位 */
        *((VOS_UINT32 *)(&pstTmpAssistData->stGpsAssist))       = 0;
        pstTmpAssistData->stGpsAssist.bitOpDgpsCorrection       = VOS_TRUE;
        pstTmpAssistData->stGpsAssist.bitOpGpsRealTimeIntegrity = VOS_TRUE;

        /* 将本次循环所对应的节点移到对应结构体的第一个位置 */
        pstTmpAssistData->stGpsAssist.stDgpsCorrection.ulDgpsCorrectionNum = 1;
        PS_MEM_CPY(&pstTmpAssistData->stGpsAssist.stDgpsCorrection.astUpDgpsCorrection[0],
                   &pstGpsAssist->stDgpsCorrection.astUpDgpsCorrection[ulLoop + MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET],
                   sizeof(AGPS_UP_DGPS_CORRECTION_STRU));

        pstTmpAssistData->stGpsAssist.stGpsRealTimeIntegrity.ulBadSatListBadSatListNum = 1;
        pstTmpAssistData->stGpsAssist.stGpsRealTimeIntegrity.ausBadSatListBadSatList[0]
                   = pstGpsAssist->stGpsRealTimeIntegrity.ausBadSatListBadSatList[ulLoop + MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET];

        /* 发送<assist_data> */
        ulResult = TAF_MTA_ProcAssistData(pstTmpAssistData);
        if (VOS_OK != ulResult)
        {
            PS_MEM_FREE(UEPS_PID_MTA, pstTmpAssistData);
            return ulResult;
        }
    }

    /* 第二轮循环 */
    for (ulLoop = 0; ulLoop < ulSecondLoop; ulLoop++)
    {
        pstTmpAssistData->bitOpGpsAssist = VOS_TRUE;
        /* 先将bit位全清为0后，再根据节点数设置<DGPS_corrections>或<GPS_rt_integrity>节点的bit位 */
        *((VOS_UINT32 *)(&pstTmpAssistData->stGpsAssist)) = 0;

        if (ulRemainDgpsCorrectionNum >= ulRemainBadSatListBadSatListNum)
        {
            pstTmpAssistData->stGpsAssist.bitOpDgpsCorrection = VOS_TRUE;

            /* 将本次循环所对应的节点移到对应结构体的第一个位置 */
            pstTmpAssistData->stGpsAssist.stDgpsCorrection.ulDgpsCorrectionNum = 1;
            PS_MEM_CPY(&pstTmpAssistData->stGpsAssist.stDgpsCorrection.astUpDgpsCorrection[0],
                       &pstGpsAssist->stDgpsCorrection.astUpDgpsCorrection[ulLoop + MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET + ulFirstLoop],
                       sizeof(AGPS_UP_DGPS_CORRECTION_STRU));
        }
        else
        {
            pstTmpAssistData->stGpsAssist.bitOpGpsRealTimeIntegrity = VOS_TRUE;

            /* 将本次循环所对应的节点移到对应结构体的第一个位置 */
            pstTmpAssistData->stGpsAssist.stGpsRealTimeIntegrity.ulBadSatListBadSatListNum = 1;
            pstTmpAssistData->stGpsAssist.stGpsRealTimeIntegrity.ausBadSatListBadSatList[0]
                    = pstGpsAssist->stGpsRealTimeIntegrity.ausBadSatListBadSatList[ulLoop + MTA_AGPS_CVRT_EXTRA_ASSI_DATA_OFFSET + ulFirstLoop];
        }

        /* 发送<assist_data> */
        ulResult = TAF_MTA_ProcAssistData(pstTmpAssistData);
        if (VOS_OK != ulResult)
        {
            PS_MEM_FREE(UEPS_PID_MTA, pstTmpAssistData);
            return ulResult;
        }
    }

    PS_MEM_FREE(UEPS_PID_MTA, pstTmpAssistData);
    return VOS_OK;
}


VOS_UINT32 TAF_MTA_ProcMeasureReq(AGPS_MEASURE_REQ_STRU *pstMeasureReq)
{
    XML_COMM_CTX_STRU                   stXmlCommCtx;                           /* XML树存储结构 */
    XML_COMM_NODE_STRU                 *pstNodePos;                             /* XML根节点 */
    XML_COMM_NODE_STRU                 *pstNodeMeasureReq;                      /* XML根节点 */
    VOS_UINT32                          ulResult;
    XML_COMM_RESULT_ENUM_UINT32         enXmlRslt;

    /* 局部变量初始化 */
    PS_MEM_SET(&stXmlCommCtx, 0x00, sizeof(XML_COMM_CTX_STRU));

    /* 初始化XmlCtx */
    if (XML_COMM_RESULT_SUCCEED != XML_COMM_InitXMLCtx(&stXmlCommCtx))
    {
        XML_COMM_ClearXMLCtx(&stXmlCommCtx);
        MTA_WARNING_LOG("TAF_MTA_ProcMeasureReq: WARNING: Init xmlCtx fail!");
        return VOS_ERR;
    }

    /* 调用XML树结构构造函数,构造XML结构树 */
    pstNodePos = TAF_MTA_AGPS_ConvertPos(&stXmlCommCtx);
    if (VOS_NULL_PTR == pstNodePos)
    {
        XML_COMM_ClearXMLCtx(&stXmlCommCtx);
        MTA_WARNING_LOG("TAF_MTA_ProcMeasureReq: WARNING: Create Pos fail!");
        return VOS_ERR;
    }

    /* 构造Measure Req数据结构 */
    pstNodeMeasureReq = TAF_MTA_AGPS_ConvertMeasureReq(&stXmlCommCtx, pstMeasureReq);
    if (VOS_NULL_PTR == pstNodeMeasureReq)
    {
        XML_COMM_ClearXMLCtx(&stXmlCommCtx);
        MTA_WARNING_LOG("TAF_MTA_ProcMeasureReq: WARNING: Create MeasureReq fail!");
        return VOS_ERR;
    }

    /* 构造完整的POS数结构 */
    enXmlRslt = XML_COMM_AddAChildNode(pstNodePos, pstNodeMeasureReq, VOS_NULL_PTR);
    if (XML_COMM_RESULT_SUCCEED != enXmlRslt)
    {
        XML_WARNING_LOG("XML_COMM_AddAChildNode: Error!");
    }
    /* XML树转化为XML码流，发送到AT模块 */
    ulResult = TAF_MTA_SndAtCposrInd(pstNodePos, &stXmlCommCtx);

    /* 释放XMLCTX资源 */
    XML_COMM_ClearXMLCtx(&stXmlCommCtx);

    return ulResult;
}


VOS_VOID TAF_MTA_SndAsPosCnf(
    TAF_PH_RAT_TYPE_ENUM_UINT8          enPhRatType,
    MTA_RRC_POSITION_CNF_STRU          *pstPosCnf
)
{
    VOS_UINT32                          ulLength;
    MTA_RRC_POSITION_CNF_STRU          *pstPositionCnf;

    /* 申请结构内存 */
    ulLength     = sizeof(MTA_RRC_POSITION_CNF_STRU) - VOS_MSG_HEAD_LENGTH;
    pstPositionCnf = (MTA_RRC_POSITION_CNF_STRU*)PS_ALLOC_MSG(UEPS_PID_MTA, ulLength);
    if (VOS_NULL_PTR == pstPositionCnf)
    {
        MTA_WARNING_LOG("TAF_MTA_SndAsPosCnf: WARNING: Memory malloc failed!");
        return;
    }

    /* 构造消息结构体 */
    pstPositionCnf->stMsgHeader.ulReceiverCpuId   = VOS_LOCAL_CPUID;
    pstPositionCnf->stMsgHeader.ulMsgName         = ID_MTA_RRC_MSG_POSITION_CNF;
    if (TAF_PH_RAT_GSM == enPhRatType)
    {
        pstPositionCnf->stMsgHeader.ulReceiverPid = UEPS_PID_GAS;
    }
    else
    {
        pstPositionCnf->stMsgHeader.ulReceiverPid = WUEPS_PID_WCOM;
    }
    PS_MEM_CPY(((VOS_UINT8*)pstPositionCnf + sizeof(MSG_HEADER_STRU)),
               ((VOS_UINT8*)pstPosCnf + sizeof(MSG_HEADER_STRU)),
               (sizeof(MTA_RRC_POSITION_CNF_STRU) - sizeof(MSG_HEADER_STRU)));

    /* 发送消息到AS模块 */
    if (VOS_OK != PS_SEND_MSG(UEPS_PID_MTA, pstPositionCnf))
    {
        MTA_WARNING_LOG("TAF_MTA_SndAsPosCnf():WARNING:SEND MSG FIAL");
    }
    return;
}
VOS_UINT32 TAF_MTA_SndPhySetGpsRfClockReq(
    TAF_PH_RAT_TYPE_ENUM_UINT8          enPhRatType,
    MTA_AT_CGPSCLOCK_ENUM_UINT32        enCgpsClock
)
{
    VOS_UINT32                          ulLength;
    MTA_PHY_SET_GPS_RF_CLOCK_REQ_STRU  *pstMtaPhyCgpsClockReq;

    /* 申请消息结构内存 */
    ulLength              = sizeof(MTA_PHY_SET_GPS_RF_CLOCK_REQ_STRU) - VOS_MSG_HEAD_LENGTH;
    pstMtaPhyCgpsClockReq = (MTA_PHY_SET_GPS_RF_CLOCK_REQ_STRU*)PS_ALLOC_MSG(UEPS_PID_MTA, ulLength);
    if (VOS_NULL_PTR == pstMtaPhyCgpsClockReq)
    {
        MTA_ERROR_LOG("TAF_MTA_SndPhySetGpsRfClockReq: Alloc msg fail!");
        return VOS_ERR;
    }

    /* 构造消息结构体 */
    pstMtaPhyCgpsClockReq->ulReceiverCpuId      = VOS_LOCAL_CPUID;

    if (TAF_PH_RAT_GSM == enPhRatType)
    {
        pstMtaPhyCgpsClockReq->ulReceiverPid    = DSP_PID_GPHY;
        pstMtaPhyCgpsClockReq->usMsgID          = ID_MTA_GPHY_SET_GPS_RF_CLOCK_REQ;
    }
    else
    {
        pstMtaPhyCgpsClockReq->ulReceiverPid    = DSP_PID_WPHY;
        pstMtaPhyCgpsClockReq->usMsgID          = ID_MTA_WPHY_SET_GPS_RF_CLOCK_REQ;
    }

    if (MTA_AT_CGPSCLOCK_STOP == enCgpsClock)
    {
        pstMtaPhyCgpsClockReq->enGpsClockState = MTA_PHY_GPS_RF_CLOCK_STATE_STOP;
    }
    else
    {
        pstMtaPhyCgpsClockReq->enGpsClockState = MTA_PHY_GPS_RF_CLOCK_STATE_RUNNING;
    }

    /* 发送消息到DSP模块 */
    if (VOS_OK != PS_SEND_MSG(UEPS_PID_MTA, pstMtaPhyCgpsClockReq))
    {
        MTA_WARNING_LOG("TAF_MTA_SndPhySetGpsRfClockReq():WARNING:SEND MSG FIAL");
    }
    return VOS_OK;
}


VOS_UINT32 TAF_MTA_FreeAgpsXmlCtx(
    TAF_MTA_AGPS_CTX_STRU              *pstAgpsCtx)
{
    /* 释放AGPS XML码流上下文 */
    if (VOS_NULL_PTR != pstAgpsCtx->stXmlText.pcBufHead)
    {
        PS_MEM_FREE(UEPS_PID_MTA, pstAgpsCtx->stXmlText.pcBufHead);
        pstAgpsCtx->stXmlText.pcBufHead = VOS_NULL_PTR;
        pstAgpsCtx->stXmlText.pcBufCur  = VOS_NULL_PTR;
    }
    return VOS_OK;
}
VOS_UINT32 TAF_MTA_DealCposSetReqAbnormal(
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg,
    XML_COMM_CTX_STRU                  *pstXmlCtx,
    TAF_MTA_AGPS_CTX_STRU              *pstAgpsCtx
)
{
    /* 释放AGPS XML码流上下文 */
    TAF_MTA_FreeAgpsXmlCtx(pstAgpsCtx);

    /* 释放XML解析申请的内存 */
    if (VOS_NULL_PTR != pstXmlCtx)
    {
        XML_COMM_ClearXMLCtx(pstXmlCtx);
    }

    /* 给AT模块回复错误消息 */
    TAF_MTA_SndAtCposSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_ERROR);

    return VOS_OK;
}
VOS_UINT32 TAF_MTA_MgmtCposTimer(
    VOS_UINT32                          ulTimerFlg
)
{
    if (VOS_OK != ulTimerFlg)
    {
        if (TAF_MTA_TIMER_STATUS_RUNING == TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AT_SET_CPOS_CNF))
        {
            TAF_MTA_StopTimer(TI_TAF_MTA_WAIT_AT_SET_CPOS_CNF);
        }

        /* 启动2s保护定时器 */
        TAF_MTA_StartTimer(TI_TAF_MTA_WAIT_AT_SET_CPOS_CNF, TI_TAF_MTA_CPOS_TIMER_LEN);
    }
    else
    {
        /* 停止2s保护定时器 */
        if (TAF_MTA_TIMER_STATUS_RUNING == TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AT_SET_CPOS_CNF))
        {
            TAF_MTA_StopTimer(TI_TAF_MTA_WAIT_AT_SET_CPOS_CNF);
        }
    }

    return VOS_OK;
}


VOS_UINT32 TAF_MTA_SaveXmlText(
    TAF_MTA_AGPS_CTX_STRU              *pstAgpsCtx,
    VOS_CHAR                           *pcXml,
    VOS_UINT32                          ulXmlLen
)
{
    VOS_UINT32                          ulXmlCount;
    /* 第一次存储XML码流，为其分配  8193 字节内存(最后一个字节存放字符串结束符'\0')*/
    if (VOS_NULL_PTR == pstAgpsCtx->stXmlText.pcBufHead)
    {
        pstAgpsCtx->stXmlText.pcBufHead = PS_MEM_ALLOC(UEPS_PID_MTA,
                                                       MTA_XML_TEXT_MAX_BUFF_SIZE + 1);
        if (VOS_NULL_PTR == pstAgpsCtx->stXmlText.pcBufHead)
        {
            MTA_WARNING_LOG("TAF_MTA_SaveXmlText: WARNING: Memory malloc failed!");
            return VOS_ERR;
        }

        pstAgpsCtx->stXmlText.pcBufCur = pstAgpsCtx->stXmlText.pcBufHead;
    }

    /* 若码流长度大于XML最大允许接收长度，则返回VOS_ERR */
    ulXmlCount = (pstAgpsCtx->stXmlText.pcBufCur - pstAgpsCtx->stXmlText.pcBufHead) + ulXmlLen;
    if (MTA_XML_TEXT_MAX_BUFF_SIZE < ulXmlCount)
    {
        MTA_WARNING_LOG("TAF_MTA_SaveXmlText: WARNING: Xml text is longer than memory!");
        return VOS_ERR;
    }

    /* 保存XML码流 */
    PS_MEM_CPY(pstAgpsCtx->stXmlText.pcBufCur,
               pcXml,
               ulXmlLen);

    pstAgpsCtx->stXmlText.pcBufCur += ulXmlLen;

    /* XML码流的结尾置为NULL */
    *pstAgpsCtx->stXmlText.pcBufCur = '\0';

    return VOS_OK;

}


VOS_UINT32 TAF_MTA_CheckXmlEnd(
    TAF_MTA_AGPS_CTX_STRU              *pstAgpsCtx
)
{
    VOS_CHAR                           *pcBufCur;

    /* 从XML码流的末尾开始查找第一个'>' */
    pcBufCur = pstAgpsCtx->stXmlText.pcBufCur - 1;
    /*lint -e961*/
    while(('>' != *(pcBufCur--)))
    {
        if (pstAgpsCtx->stXmlText.pcBufHead == pcBufCur)
        {
            return VOS_ERR;
        }
    }

    /* 从XML码流的末尾开始查找第一个'<' */
    while(('<' != *(pcBufCur--)))
    {
        if (pstAgpsCtx->stXmlText.pcBufHead == pcBufCur)
        {
            return VOS_ERR;
        }
    }
    /*lint +e961*/

    /* 对比标签名称 */
    pcBufCur += 1;
    if (VOS_OK == PS_MEM_CMP(MTA_AGPS_LABEL_POS_END, pcBufCur, VOS_StrLen(MTA_AGPS_LABEL_POS_END)))
    {
        pcBufCur += VOS_StrLen(MTA_AGPS_LABEL_POS_END);

        /* "</pos"后面跟的字符后可跟分割符，同样视为合法码流 */
        while(pstAgpsCtx->stXmlText.pcBufCur != pcBufCur)
        {
            if ((VOS_TRUE != XML_COMM_IsSeparator(*pcBufCur))
             && ('>' != *pcBufCur))
            {
                return VOS_ERR;
            }
            pcBufCur++;
        }

        return VOS_OK;
    }

    return VOS_ERR;
}


VOS_UINT32 TAF_MTA_FillXmlStru(
    XML_COMM_NODE_STRU                 *pstXmlNode,
    MTA_RRC_POSITION_CNF_STRU          *pstPositionCnf
)
{
    XML_COMM_NODE_STRU                 *pstXmlFirstNode;
    VOS_UINT32                          ulResult;

    ulResult = VOS_OK;

    /* 获取子节点数 */
    if (MTA_AGPS_XML_NODE_COUNT_1 != XML_COMM_GetChildCount(pstXmlNode))
    {
        return VOS_ERR;
    }

    /*获取子节点 */
    pstXmlFirstNode = XML_COMM_GetChildNodeByIndex(pstXmlNode, 0);
    if (VOS_NULL_PTR ==pstXmlFirstNode)
    {
        return VOS_ERR;
    }

    /* 填写数据结构 */
    if (VOS_TRUE == XML_COMM_CheckLabelName(pstXmlFirstNode, MTA_AGPS_LABEL_LOCATION))
    {
        ulResult = TAF_MTA_AGPS_FillLocationInfo(pstXmlFirstNode, &pstPositionCnf->u.stLocationInfo);
        pstPositionCnf->enResultTypeChoice  = AGPS_POSITION_RESULT_TYPE_LOCATION;
    }
    else if (VOS_TRUE == XML_COMM_CheckLabelName(pstXmlFirstNode, MTA_AGPS_LABEL_GPS_MEAS))
    {
        ulResult = TAF_MTA_AGPS_FillGpsMeas(pstXmlFirstNode, &pstPositionCnf->u.stGpsMeasInfo);
        pstPositionCnf->enResultTypeChoice  = AGPS_POSITION_RESULT_TYPE_MEAS;
    }
    else if ( VOS_TRUE == XML_COMM_CheckLabelName(pstXmlFirstNode, MTA_AGPS_LABEL_GPS_ASSIST_REQ))
    {
        ulResult = TAF_MTA_AGPS_FillGpsAssistReq(pstXmlFirstNode, &pstPositionCnf->u.stAssistDataReq);
        pstPositionCnf->enResultTypeChoice  = AGPS_POSITION_RESULT_TYPE_ASSIST_REQ;
    }
    else if (VOS_TRUE == XML_COMM_CheckLabelName(pstXmlFirstNode, MTA_AGPS_LABEL_POS_ERR))
    {
        ulResult = TAF_MTA_AGPS_FillPosErr(pstXmlFirstNode, &pstPositionCnf->u.stPosErr);
        pstPositionCnf->enResultTypeChoice  = AGPS_POSITION_RESULT_TYPE_ERROR;
    }
    else
    {
        return VOS_ERR;
    }

    if (VOS_OK != ulResult)
    {
       return VOS_ERR;
    }

    return VOS_OK;
}
#endif /* FEATURE_AGPS */


VOS_VOID TAF_MTA_SndAtCposSetCnf (
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg,
    MTA_AT_RESULT_ENUM_UINT32           enErr
)
{
    MTA_AT_CPOS_CNF_STRU                stMtaAtCposCnf;

    /* 发送消息给AT模块 */
    stMtaAtCposCnf.enResult = enErr;

    TAF_MTA_SndAtMsg( &pstAtMtaReqMsg->stAppCtrl,
                      ID_MTA_AT_CPOS_SET_CNF,
                      sizeof(MTA_AT_CPOS_CNF_STRU),
                      (VOS_UINT8*)&stMtaAtCposCnf );
    return;
}
VOS_VOID TAF_MTA_SndAtCgpsClockSetCnf(
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg,
    AT_MTA_MSG_TYPE_ENUM_UINT32         enErr)
{
    MTA_AT_CGPSCLOCK_CNF_STRU           stMtaAtCgpsClockCnf;

    /* 发送结果给AT模块 */
    stMtaAtCgpsClockCnf.enResult = enErr;

    TAF_MTA_SndAtMsg( &pstAtMtaReqMsg->stAppCtrl,
                      ID_MTA_AT_CGPSCLOCK_SET_CNF,
                      sizeof(MTA_AT_CGPSCLOCK_CNF_STRU),
                      (VOS_UINT8*)&stMtaAtCgpsClockCnf );
    return;
}
VOS_VOID TAF_MTA_RcvAtCposSetReq(VOS_VOID *pMsg)
{
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg;                     /* 收到AT发来的消息指针 */
#if (FEATURE_ON == FEATURE_AGPS)
    VOS_UINT32                          ulResult;
    XML_COMM_CTX_STRU                   stXmlCtx;
    XML_COMM_NODE_STRU                 *pstXmlNode;
    AT_MTA_CPOS_REQ_STRU               *pstAtMtaCposReq;
    TAF_MTA_AGPS_CTX_STRU              *pstAgpsCtx;
    MTA_RRC_POSITION_CNF_STRU          *pstPositionCnf;

    TAF_SDC_SYS_MODE_ENUM_UINT8         enRatType;                              /* 当前驻留模式 */

    enRatType = TAF_SDC_GetSysMode();

#endif

    /* 局部变量初始化 */
    pstAtMtaReqMsg  = (AT_MTA_MSG_STRU*)pMsg;

#if (FEATURE_ON == FEATURE_AGPS)
    pstAtMtaCposReq = (AT_MTA_CPOS_REQ_STRU*)pstAtMtaReqMsg->aucContent;
    pstAgpsCtx      = VOS_NULL_PTR;
    PS_MEM_SET(&stXmlCtx, 0x00, sizeof(XML_COMM_CTX_STRU));

    /* 获取AGPS XML资源上下文 */
    pstAgpsCtx = TAF_MTA_GetMtaAgpsCtxAddr();

    /* 若当前CPOS操作为CANCEL，则释放之前申请的内存，并给AT返回消息 */
    if (MTA_AT_CPOS_CANCEL == pstAtMtaCposReq->enCposOpType)
    {
        TAF_MTA_FreeAgpsXmlCtx(pstAgpsCtx);
        TAF_MTA_SndAtCposSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_NO_ERROR);
        return;
    }

    /* 存储XML码流 */
    ulResult = TAF_MTA_SaveXmlText(pstAgpsCtx,
                                   pstAtMtaCposReq->acXmlText,
                                   pstAtMtaCposReq->ulXmlLength);
    if (VOS_OK != ulResult)
    {
        TAF_MTA_DealCposSetReqAbnormal(pstAtMtaReqMsg, VOS_NULL_PTR, pstAgpsCtx);
        MTA_WARNING_LOG("TAF_MTA_RcvAtCposSetReq: WARNING: Xml text Saving failed!");
        return;
    }

    /* 判断当前XML码流是否完整，以判断结果作为启停保护定时器的开关 */
    ulResult = TAF_MTA_CheckXmlEnd(pstAgpsCtx);

    /* 若完整则停止2s保护定时器；否则先关闭定时器，再重新启动2S定时器 */
    TAF_MTA_MgmtCposTimer(ulResult);

    /* XML码流不完整，给AT模块回复消息 */
    if (VOS_OK != ulResult)
    {
        TAF_MTA_SndAtCposSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_NO_ERROR);
        return;
    }

    /* 获取当前驻留的模式,若当前模式不为G/W,返回ERROR */

    if ((TAF_SDC_SYS_MODE_GSM != enRatType)
     && (TAF_SDC_SYS_MODE_WCDMA != enRatType))
    {
        TAF_MTA_DealCposSetReqAbnormal(pstAtMtaReqMsg, VOS_NULL_PTR, pstAgpsCtx);
        MTA_WARNING_LOG("TAF_MTA_RcvAtCposSetReq: WARNING: RatType is not G or W!");
        return;
    }

    /* 调用XML码流解析函数,构造XML结构树 */
    if (XML_COMM_RESULT_SUCCEED != XML_COMM_InitXMLCtx(&stXmlCtx))
    {
        TAF_MTA_DealCposSetReqAbnormal(pstAtMtaReqMsg, &stXmlCtx, pstAgpsCtx);
        MTA_WARNING_LOG("TAF_MTA_RcvAtCposSetReq: WARNING: Xml initialization failed!");
        return;
    }
    pstXmlNode = XML_COMM_BuildXMLTree( pstAgpsCtx->stXmlText.pcBufHead,
                                        &stXmlCtx);

    if (VOS_NULL_PTR == pstXmlNode)
    {
        TAF_MTA_DealCposSetReqAbnormal(pstAtMtaReqMsg, &stXmlCtx, pstAgpsCtx);
        MTA_WARNING_LOG("TAF_MTA_RcvAtCposSetReq: WARNING: Build xml tree failed!");
        return;
    }

    /* 申请结构内存 */
    pstPositionCnf = (MTA_RRC_POSITION_CNF_STRU*)PS_MEM_ALLOC(UEPS_PID_MTA,
                                                            sizeof(MTA_RRC_POSITION_CNF_STRU));
    if (VOS_NULL_PTR == pstPositionCnf)
    {
        TAF_MTA_DealCposSetReqAbnormal(pstAtMtaReqMsg, &stXmlCtx, pstAgpsCtx);
        MTA_WARNING_LOG("TAF_MTA_RcvAtCposSetReq: WARNING: Memory malloc failed!");
        return;
    }
    PS_MEM_SET(pstPositionCnf, 0x00, sizeof(MTA_RRC_POSITION_CNF_STRU));

    /* 调用各个标签构造函数，为消息结构申请内存，填写XML数据结构 */
    ulResult = TAF_MTA_FillXmlStru(pstXmlNode, pstPositionCnf);

    /* 如果构造失败，则释放申请的内存，给AT模块返回VOS_ERR */
    if (VOS_OK != ulResult)
    {
        PS_MEM_FREE(UEPS_PID_MTA, pstPositionCnf);
        TAF_MTA_DealCposSetReqAbnormal(pstAtMtaReqMsg, &stXmlCtx, pstAgpsCtx);
        MTA_WARNING_LOG("TAF_MTA_RcvAtCposSetReq: WARNING: Fill xml structure failed!");
        return;
    }

    /* 发送消息给接入层 */
    TAF_MTA_SndAsPosCnf(enRatType, pstPositionCnf);

    /* 释放分配的内存 */
    XML_COMM_ClearXMLCtx(&stXmlCtx);
    TAF_MTA_FreeAgpsXmlCtx(pstAgpsCtx);
    PS_MEM_FREE(UEPS_PID_MTA, pstPositionCnf);

    /* 给AT模块回复消息 */
    TAF_MTA_SndAtCposSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_NO_ERROR);
    return;
#else
    /* FEATURE_AGPS特性未打开，打印告警日志，给AT模块回复错误消息 */
    TAF_MTA_SndAtCposSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_ERROR);
    MTA_WARNING_LOG("TAF_MTA_RcvRrcMsgPositionReq: WARNING: Msg is unexpected!");
    return;
#endif
}


VOS_VOID TAF_MTA_RcvTiCposSetReqExpired(VOS_VOID *pMsg)
{
#if (FEATURE_ON == FEATURE_AGPS)
    TAF_MTA_AGPS_CTX_STRU                  *pstAgpsCtx;

    /* 释放申请的资源 */
    pstAgpsCtx = TAF_MTA_GetMtaAgpsCtxAddr();

    /* 释放AGPS XML码流上下文 */
    if (VOS_NULL_PTR != pstAgpsCtx->stXmlText.pcBufHead)
    {
        PS_MEM_FREE(UEPS_PID_MTA, pstAgpsCtx->stXmlText.pcBufHead);
        pstAgpsCtx->stXmlText.pcBufHead = VOS_NULL_PTR;
        pstAgpsCtx->stXmlText.pcBufCur  = VOS_NULL_PTR;
    }

    return;
#else
    /* FEATURE_AGPS特性未打开，打印告警日志，忽略该消息 */
    MTA_WARNING_LOG("TAF_MTA_RcvTiCposSetReqExpired: WARNING: Msg is unexpected!");
    return;
#endif
}
VOS_VOID TAF_MTA_RcvRrcMsgPositionReq(VOS_VOID *pMsg)
{
#if (FEATURE_ON == FEATURE_AGPS)
    MTA_RRC_POSITION_REQ_STRU          *pstMtaRrcPositionReqMsg;           /* 接收消息指针 */
    VOS_UINT32                          ulResult;

    TAF_SDC_SYS_MODE_ENUM_UINT8         enRatType;                              /* 当前驻留模式 */

    enRatType = TAF_SDC_GetSysMode();

    /* 局部变量初始化 */
    pstMtaRrcPositionReqMsg = (MTA_RRC_POSITION_REQ_STRU*)pMsg;
    ulResult                = VOS_ERR;

    /* 获取当前驻留的模式,若当前模式不为G/W,返回ERROR */
    if ((TAF_SDC_SYS_MODE_GSM != enRatType)
     && (TAF_SDC_SYS_MODE_WCDMA != enRatType))
    {
        MTA_WARNING_LOG("TAF_MTA_RcvRrcMsgPositionReq: WARNING: RatType is not G or W!");
        return;
    }

    /* 判断是否上报清空辅助数据指示 */
    if (PS_TRUE == pstMtaRrcPositionReqMsg->enDeleteAssistDataFlag)
    {
        ulResult = TAF_MTA_SndAtXcposrRptInd((VOS_UINT32)pstMtaRrcPositionReqMsg->enDeleteAssistDataFlag);
    }

    /* 判断是否主动上报辅助数据 */
    if (VOS_TRUE == pstMtaRrcPositionReqMsg->bitOpAssistData)
    {
        ulResult = TAF_MTA_ProcAssistData(&pstMtaRrcPositionReqMsg->stAssistData);
        if (VOS_OK != ulResult)
        {
            MTA_WARNING_LOG("TAF_MTA_RcvRrcMsgPositionReq: WARNING: Report AssistData failed!");
            return;
        }

        /* <assist_data>中的<GPS_assist>节点中，<DGPS_corrections>和<GPS_rt_integrity>子节点在27007 XML DTD规范中
           都是单个的，但接入层上报的结构体中则是多个的。基于此，MTA需要做特殊处理: 若<DGPS_corrections>或
           <GPS_rt_integrity>多于1个，则首次上报的<assist_data> XML中，只上报第一个，剩余的则拆分上报，每次
           <assist_data>上报中带一个 */
        ulResult = TAF_MTA_ProcExtraAssistData(&pstMtaRrcPositionReqMsg->stAssistData);
        if (VOS_OK != ulResult)
        {
            MTA_WARNING_LOG("TAF_MTA_RcvRrcMsgPositionReq: WARNING: Report ExtrAssistData failed!");
            return;
        }
    }

    /* 判断是否主动上报测量指示*/
    if (VOS_TRUE == pstMtaRrcPositionReqMsg->bitOpPosMeas)
    {
        ulResult = TAF_MTA_ProcMeasureReq(&pstMtaRrcPositionReqMsg->stPosMeas);
        if (VOS_OK != ulResult)
        {
            MTA_WARNING_LOG("TAF_MTA_RcvRrcMsgPositionReq: WARNING: Report MeasureReq failed!");
            return;
        }
    }

    return;

#else
    /* FEATURE_AGPS特性未打开，打印告警日志，忽略该消息 */
    MTA_WARNING_LOG("TAF_MTA_RcvRrcMsgPositionReq: WARNING: Msg is unexpected!");
    return;

#endif
}


VOS_VOID TAF_MTA_RcvAtCgpsClockSetReq(VOS_VOID *pMsg)
{

    AT_MTA_MSG_STRU                        *pstAtMtaReqMsg;                     /* 收到AT发来的消息指针 */
#if (FEATURE_ON == FEATURE_AGPS)
#if 0
    AT_MTA_CGPSCLOCK_REQ_STRU              *pstAtMtaCgpsClockReq;               /* 启停时钟 */
    TAF_PH_RAT_TYPE_ENUM_UINT8              enRatType;                          /* 当前驻留模式 */
    TAF_MTA_TIMER_ID_ENUM_UINT32            enMtaTimerId;                       /* 定时器ID */
#endif
#endif

    /* 局部变量初始化 */
    pstAtMtaReqMsg                          = (AT_MTA_MSG_STRU*)pMsg;

#if (FEATURE_ON == FEATURE_AGPS)

#if 1
    /* 目前，DSP的时钟为上电即启动，为了和AP对接，当AT模块下发AT^CGPSCLOCK命令时，
    在MTA模块直接给AT模块返回MTA_AT_RESULT_NO_ERROR */
    TAF_MTA_SndAtCgpsClockSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_NO_ERROR);
    return;

#else /* 本函数以下代码保留，以便后期AP变更GPS时钟方案时，可直接利用 */
    pstAtMtaCgpsClockReq                    = (AT_MTA_CGPSCLOCK_REQ_STRU *)pstAtMtaReqMsg->aucContent;
    enMtaTimerId                            = TI_TAF_MTA_TIMER_BUTT;

    /* 获取当前驻留的模式,若当前模式不为G或W模，则给AT返回错误值 */
    enRatType = MN_PH_GetCurRat();

    if (TAF_PH_RAT_GSM == enRatType)
    {
        enMtaTimerId = TI_TAF_MTA_WAIT_GPHY_SET_CGPSCLOCK_CNF;
    }
    else if (TAF_PH_RAT_WCDMA == enRatType)
    {
        enMtaTimerId = TI_TAF_MTA_WAIT_WPHY_SET_CGPSCLOCK_CNF;
    }
    else
    {
        TAF_MTA_SndAtCgpsClockSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_ERROR);
        return;
    }

    /* 如果当前定时器已启动，则返回VOS_ERR */
    if (TAF_MTA_TIMER_STATUS_RUNING == TAF_MTA_GetTimerStatus(enMtaTimerId))
    {
        TAF_MTA_SndAtCgpsClockSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_ERROR);
        return;
    }

    /* 发送消息给DSP模块 */
    if (VOS_OK != TAF_MTA_SndPhySetGpsRfClockReq(enRatType, pstAtMtaCgpsClockReq->enGpsClockState))
    {
        return;
    }
    /* 启动1s保护定时器 */
    TAF_MTA_StartTimer(enMtaTimerId, TI_TAF_MTA_GPS_TIMER_LEN);

    /* 添加消息进等待队列 */
    TAF_MTA_SaveItemInCmdBufferQueue(enMtaTimerId,
                                     (VOS_UINT8*)&pstAtMtaReqMsg->stAppCtrl,
                                     sizeof(AT_APPCTRL_STRU));

    return;
#endif

#else
    /* FEATURE_AGPS特性未打开，打印告警日志，给AT模块回复错误消息 */
    TAF_MTA_SndAtCgpsClockSetCnf(pstAtMtaReqMsg, MTA_AT_RESULT_ERROR);

    MTA_WARNING_LOG("TAF_MTA_RcvAtCgpsClockSetReq: WARNING: Msg is unexpected!");
    return;

#endif
}
VOS_VOID TAF_MTA_RcvPhyCgpsClockSetCnf(VOS_VOID *pMsg)
{
#if (FEATURE_ON == FEATURE_AGPS)
    TAF_MTA_CMD_BUFFER_STRU                 *pstCmdBuf;
    MTA_AT_CGPSCLOCK_CNF_STRU                stAtMtaCgpsClockCnf;               /* 启停时钟命令执行结果 */
    MTA_PHY_SET_GPS_RF_CLOCK_CNF_STRU       *pstMtaPhyCgpsClockCnf;             /* 接收到DSP返回的消息结构 */
    TAF_MTA_TIMER_ID_ENUM_UINT32             enTimerId;

    TAF_SDC_SYS_MODE_ENUM_UINT8               enRatType;                         /* 当前驻留模式 */

    enRatType = TAF_SDC_GetSysMode();

    /* 局部变量初始化 */
    pstMtaPhyCgpsClockCnf        = (MTA_PHY_SET_GPS_RF_CLOCK_CNF_STRU*)pMsg;

    /* 获取当前驻留的模式,若当前模式为LTE */
    if (TAF_SDC_SYS_MODE_GSM == enRatType)
    {
        enTimerId = TI_TAF_MTA_WAIT_GPHY_SET_CGPSCLOCK_CNF;
    }
    else if (TAF_SDC_SYS_MODE_WCDMA == enRatType)
    {
        enTimerId = TI_TAF_MTA_WAIT_WPHY_SET_CGPSCLOCK_CNF;
    }
    else
    {
        MTA_WARNING_LOG("TAF_MTA_RcvPhyCgpsClockSetCnf: WARNING: RatType is unexpected!");
        return;
    }


    /* 如果当前定时器不为运行状态 */
    if (TAF_MTA_TIMER_STATUS_RUNING != TAF_MTA_GetTimerStatus(enTimerId))
    {
        MTA_WARNING_LOG("TAF_MTA_RcvPhyCgpsClockSetCnf: WARNING: Timer was already stop!");
        return;
    }

    /* 停止保护定时器 */
    TAF_MTA_StopTimer(enTimerId);

    /* 获取当前定时器对应的消息队列 */
    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(enTimerId);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        return;
    }

    /* 构造消息结构体 */
    if (MTA_PHY_RESULT_NO_ERROR == pstMtaPhyCgpsClockCnf->enResultType)
    {
        stAtMtaCgpsClockCnf.enResult = MTA_AT_RESULT_NO_ERROR;
    }
    else
    {
        stAtMtaCgpsClockCnf.enResult = MTA_AT_RESULT_ERROR;
    }

    /* 发送消息给AT模块 */
    TAF_MTA_SndAtMsg( (AT_APPCTRL_STRU*)pstCmdBuf->pucMsgInfo,
                      ID_MTA_AT_CGPSCLOCK_SET_CNF,
                      sizeof(MTA_AT_CGPSCLOCK_CNF_STRU),
                      (VOS_UINT8*)&stAtMtaCgpsClockCnf );

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;
#else
    /* FEATURE_AGPS特性未打开，打印告警日志，忽略该消息 */
    MTA_WARNING_LOG("TAF_MTA_RcvPhyCgpsClockSetCnf: WARNING: Msg is unexpected!");
    return;
#endif
}


VOS_VOID TAF_MTA_RcvTiCgpsClockSetReqExpired(VOS_VOID *pMsg)
{
#if (FEATURE_ON == FEATURE_AGPS)
    TAF_MTA_CMD_BUFFER_STRU                *pstCmdBuf;
    MTA_AT_CGPSCLOCK_CNF_STRU               stAtMtaCgpsClockCnf;               /* 启停时钟命令执行结果 */
    TAF_MTA_TIMER_ID_ENUM_UINT32            enTimerId;

    /* 获取当前驻留的模式,若当前模式不为G/W,返回ERROR */
    TAF_SDC_SYS_MODE_ENUM_UINT8              enRatType;                        /* 当前驻留模式 */
    enRatType = TAF_SDC_GetSysMode();

    if (TAF_SDC_SYS_MODE_GSM == enRatType)
    {
        enTimerId = TI_TAF_MTA_WAIT_GPHY_SET_CGPSCLOCK_CNF;
    }
    else if (TAF_SDC_SYS_MODE_WCDMA == enRatType)
    {
        enTimerId = TI_TAF_MTA_WAIT_WPHY_SET_CGPSCLOCK_CNF;
    }
    else
    {
        MTA_WARNING_LOG("TAF_MTA_RcvTiCgpsClockSetReqExpired: WARNING: RatType is not G or W!");
        return;
    }


    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(enTimerId);

    if (VOS_NULL_PTR == pstCmdBuf)
    {
        MTA_WARNING_LOG("TAF_MTA_RcvTiCgpsClockSetReqExpired: WARNING: buffer is null!");
        return;
    }

    /* 构造消息结构体 */
    stAtMtaCgpsClockCnf.enResult = MTA_AT_RESULT_ERROR;

    /* 发送消息给AT模块 */
    TAF_MTA_SndAtMsg( (AT_APPCTRL_STRU*)pstCmdBuf->pucMsgInfo,
                      ID_MTA_AT_CGPSCLOCK_SET_CNF,
                      sizeof(MTA_AT_CGPSCLOCK_CNF_STRU),
                      (VOS_UINT8*)&stAtMtaCgpsClockCnf );

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;
#else
    /* FEATURE_AGPS特性未打开，打印告警日志，忽略该消息 */
    MTA_WARNING_LOG("TAF_MTA_RcvTiCgpsClockSetReqExpired: WARNING: Msg is unexpected!");
    return;
#endif
}
VOS_VOID TAF_MTA_RcvTcResetUePosStoredInfoInd(VOS_VOID *pMsg)
{
#if (FEATURE_ON == FEATURE_AGPS)
    /* 向AT发送辅助数据清除指示 */
    TAF_MTA_SndAtXcposrRptInd(VOS_TRUE);

    return;
#else
    /* FEATURE_AGPS特性未打开，打印告警日志，忽略该消息 */
    MTA_WARNING_LOG("TAF_MTA_RcvTcResetUePosStoredInfoInd: WARNING: Msg is unexpected!");
    return;
#endif
}


VOS_VOID TAF_MTA_RcvTiWaitAsQryNmrCnfExpired(VOS_VOID *pMsg)
{
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf = VOS_NULL_PTR;
    MTA_AT_QRY_NMR_CNF_STRU             stQryNmrCnf;

    /* 获取当前定时器对应的消息队列 */
    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);

    if (VOS_NULL_PTR == pstCmdBuf)
    {
        return;
    }

    PS_MEM_SET(&stQryNmrCnf, 0x0, sizeof(stQryNmrCnf));

    stQryNmrCnf.enResult = MTA_AT_RESULT_ERROR;

    /* 把接入层上报的nmr数据上报给AT模块 */
    TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                     ID_MTA_AT_QRY_NMR_CNF,
                     sizeof(stQryNmrCnf),
                     (VOS_UINT8*)&stQryNmrCnf );


    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;
}
VOS_UINT32 TAF_MTA_SndAsQryNmrReqMsg(
    VOS_UINT32                          ulReceiverPid
)
{
    VOS_UINT32                          ulLength;
    MTA_RRC_QRY_NMR_REQ_STRU           *pstMtaAsQryNmrReq = VOS_NULL_PTR;

    /* 申请消息结构内存 */
    ulLength              = sizeof(MTA_RRC_QRY_NMR_REQ_STRU) - VOS_MSG_HEAD_LENGTH;
    pstMtaAsQryNmrReq     = (MTA_RRC_QRY_NMR_REQ_STRU*)PS_ALLOC_MSG(UEPS_PID_MTA, ulLength);
    if (VOS_NULL_PTR == pstMtaAsQryNmrReq)
    {
        MTA_ERROR_LOG("TAF_MTA_SndAsQryNmrReqMsg: Alloc msg fail!");
        return VOS_ERR;
    }

    /* 构造消息结构体 */
    pstMtaAsQryNmrReq->MsgHeader.ulSenderCpuId     = VOS_LOCAL_CPUID;
    pstMtaAsQryNmrReq->MsgHeader.ulSenderPid       = UEPS_PID_MTA;
    pstMtaAsQryNmrReq->MsgHeader.ulReceiverCpuId   = VOS_LOCAL_CPUID;
    pstMtaAsQryNmrReq->MsgHeader.ulReceiverPid     = ulReceiverPid;
    pstMtaAsQryNmrReq->MsgHeader.ulMsgName         = ID_MTA_RRC_QRY_NMR_REQ;

    PS_MEM_SET(pstMtaAsQryNmrReq->aucReserve, 0x0, sizeof(pstMtaAsQryNmrReq->aucReserve));

    /* 发送消息到AS */
    PS_SEND_MSG(UEPS_PID_MTA, pstMtaAsQryNmrReq);

    return VOS_OK;
}
VOS_VOID TAF_MTA_RcvQryNmrReq(VOS_VOID *pMsg)
{
    AT_MTA_MSG_STRU                    *pstQryNmrReq      = VOS_NULL_PTR;
    AT_MTA_QRY_NMR_REQ_STRU            *pstRrcAtQryNmrReq = VOS_NULL_PTR;
    MTA_AT_QRY_NMR_CNF_STRU             stQryNmrCnf;
    TAF_MTA_TIMER_STATUS_ENUM_UINT8     enQryNmrTiStatus;
    TAF_MTA_TIMER_STATUS_ENUM_UINT8     enEcidSetTiStatus;
    VOS_UINT32                          ulResult;

    pstQryNmrReq      = (AT_MTA_MSG_STRU *)pMsg;
    pstRrcAtQryNmrReq = (AT_MTA_QRY_NMR_REQ_STRU *)pstQryNmrReq->aucContent;

    /* 查询的不是2g和3g的nmr数据，或者当前接入模式与查询模式不匹配，直接返回失败*/
    if ((RRC_MTA_UTRAN_NMR != pstRrcAtQryNmrReq->ucRatType)
     && (RRC_MTA_GSM_NMR != pstRrcAtQryNmrReq->ucRatType))
    {
        PS_MEM_SET(&stQryNmrCnf, 0x0, sizeof(stQryNmrCnf));
        stQryNmrCnf.enResult = MTA_AT_RESULT_ERROR;

        TAF_MTA_SndAtMsg(&pstQryNmrReq->stAppCtrl,
                      ID_MTA_AT_QRY_NMR_CNF,
                      sizeof(stQryNmrCnf),
                      (VOS_UINT8*)&stQryNmrCnf );

        return;
    }

    /* 如果当前定时器已启动，则返回失败 */
    enQryNmrTiStatus  = TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);
    enEcidSetTiStatus = TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);
    if ( (TAF_MTA_TIMER_STATUS_RUNING == enQryNmrTiStatus)
      || (TAF_MTA_TIMER_STATUS_RUNING == enEcidSetTiStatus) )
    {
        PS_MEM_SET(&stQryNmrCnf, 0x0, sizeof(stQryNmrCnf));
        stQryNmrCnf.enResult = MTA_AT_RESULT_ERROR;

        TAF_MTA_SndAtMsg(&pstQryNmrReq->stAppCtrl,
                      ID_MTA_AT_QRY_NMR_CNF,
                      sizeof(stQryNmrCnf),
                      (VOS_UINT8*)&stQryNmrCnf );
        return;
    }

    /* 收到at的^cnmr请求，发ID_AT_MTA_QRY_NMR_REQ消息通知as */
    if (RRC_MTA_UTRAN_NMR == pstRrcAtQryNmrReq->ucRatType)
    {
        ulResult = TAF_MTA_SndAsQryNmrReqMsg(WUEPS_PID_WRR);
    }
    else
    {
        ulResult = TAF_MTA_SndAsQryNmrReqMsg(UEPS_PID_GAS);
    }

    if (ulResult != VOS_OK)
    {
        /* 消息发送失败，给at回复失败*/
        PS_MEM_SET(&stQryNmrCnf, 0x0, sizeof(stQryNmrCnf));
        stQryNmrCnf.enResult = MTA_AT_RESULT_ERROR;

        TAF_MTA_SndAtMsg(&pstQryNmrReq->stAppCtrl,
                         ID_MTA_AT_QRY_NMR_CNF,
                         sizeof(stQryNmrCnf),
                         (VOS_UINT8*)&stQryNmrCnf );
        return;
    }

    /* 启动保护定时器 */
    TAF_MTA_StartTimer(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF, TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF_TIMER_LEN);

    /* 添加消息进等待队列 */
    TAF_MTA_SaveItemInCmdBufferQueue(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF,
                                     (VOS_UINT8*)&pstQryNmrReq->stAppCtrl,
                                     sizeof(AT_APPCTRL_STRU));

    return;
}



VOS_VOID NAS_MMC_RcvWasQryNmrCnf(RRC_MTA_QRY_NMR_CNF_STRU *pstRcvQryAsNmrCnf)
{
    MTA_AT_QRY_NMR_CNF_STRU             stSndAtQryNmrCnf;
    MTA_AT_QRY_NMR_CNF_STRU            *pstSndAtQryNmrCnf = VOS_NULL_PTR;
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf         = VOS_NULL_PTR;
    VOS_UINT16                          usLen;
    VOS_UINT8                           ucLoop;
    VOS_UINT8                           ucFreqNum;

    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        return;
    }


    if (0 == pstRcvQryAsNmrCnf->u.stUtranNMRData.ulFreqNum)
    {
        stSndAtQryNmrCnf.enResult     = MTA_AT_RESULT_NO_ERROR;
        stSndAtQryNmrCnf.ucTotalIndex = 0;
        stSndAtQryNmrCnf.ucCurrIndex  = 0;
        stSndAtQryNmrCnf.usNMRLen     = 0;
        PS_MEM_SET(stSndAtQryNmrCnf.aucNMRData, 0x0, sizeof(stSndAtQryNmrCnf.aucNMRData));

        TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                         ID_MTA_AT_QRY_NMR_CNF,
                         sizeof(stSndAtQryNmrCnf),
                         (VOS_UINT8*)&stSndAtQryNmrCnf);

        /* 从等待队列中删除消息 */
        TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);
        return;
    }

    ucFreqNum = (VOS_UINT8)pstRcvQryAsNmrCnf->u.stUtranNMRData.ulFreqNum;

    usLen             = sizeof(MTA_AT_QRY_NMR_CNF_STRU) + sizeof(NMR_UTRAN_MEASURED_RESULTS_STRU) - 4;
    pstSndAtQryNmrCnf = (MTA_AT_QRY_NMR_CNF_STRU *)PS_MEM_ALLOC(UEPS_PID_MTA, usLen);

    if (VOS_NULL_PTR == pstSndAtQryNmrCnf)
    {
        MTA_ERROR_LOG("NAS_MMC_RcvWasQryNmrCnf: alloc msg fail!");

        stSndAtQryNmrCnf.enResult     = MTA_AT_RESULT_NO_ERROR;
        stSndAtQryNmrCnf.ucTotalIndex = ucFreqNum;
        stSndAtQryNmrCnf.ucCurrIndex  = ucFreqNum;
        stSndAtQryNmrCnf.usNMRLen     = 0;
        PS_MEM_SET(stSndAtQryNmrCnf.aucNMRData, 0x0, sizeof(stSndAtQryNmrCnf.aucNMRData));

        TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                         ID_MTA_AT_QRY_NMR_CNF,
                         sizeof(stSndAtQryNmrCnf),
                         (VOS_UINT8*)&stSndAtQryNmrCnf);

        /* 从等待队列中删除消息 */
        TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);
        return;
    }

    for (ucLoop = 0; ucLoop < ucFreqNum; ucLoop++)
    {
        pstSndAtQryNmrCnf->enResult     = MTA_AT_RESULT_NO_ERROR;
        pstSndAtQryNmrCnf->ucTotalIndex = ucFreqNum;
        pstSndAtQryNmrCnf->ucCurrIndex  = ucLoop + 1;
        pstSndAtQryNmrCnf->usNMRLen     = sizeof(NMR_UTRAN_MEASURED_RESULTS_STRU);

        PS_MEM_CPY(pstSndAtQryNmrCnf->aucNMRData, &pstRcvQryAsNmrCnf->u.stUtranNMRData.astMeasResults[ucLoop], pstSndAtQryNmrCnf->usNMRLen);

        TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                         ID_MTA_AT_QRY_NMR_CNF,
                         usLen,
                         (VOS_UINT8*)pstSndAtQryNmrCnf);
    }

    PS_MEM_FREE(UEPS_PID_MTA, pstSndAtQryNmrCnf);

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);
    return;

}
VOS_VOID NAS_MMC_RcvGasQryNmrCnf(RRC_MTA_QRY_NMR_CNF_STRU *pstRcvQryAsNmrCnf)
{
    MTA_AT_QRY_NMR_CNF_STRU             stSndAtQryNmrCnf;
    MTA_AT_QRY_NMR_CNF_STRU            *pstSndAtQryNmrCnf = VOS_NULL_PTR;
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf         = VOS_NULL_PTR;
    VOS_UINT16                          usLen;
    VOS_UINT8                           ucLoop;
    VOS_UINT8                           ucFreqNum;

    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        return;
    }
    if (0 == pstRcvQryAsNmrCnf->u.stGsmNMRData.ulElemNum)
    {
        stSndAtQryNmrCnf.enResult     = MTA_AT_RESULT_NO_ERROR;
        stSndAtQryNmrCnf.ucTotalIndex = 0;
        stSndAtQryNmrCnf.ucCurrIndex  = 0;
        stSndAtQryNmrCnf.usNMRLen     = 0;
        PS_MEM_SET(stSndAtQryNmrCnf.aucNMRData, 0x0, sizeof(stSndAtQryNmrCnf.aucNMRData));

        TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                         ID_MTA_AT_QRY_NMR_CNF,
                         sizeof(stSndAtQryNmrCnf),
                         (VOS_UINT8*)&stSndAtQryNmrCnf);

        /* 从等待队列中删除消息 */
        TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);
        return;
    }

    ucFreqNum = (VOS_UINT8)pstRcvQryAsNmrCnf->u.stGsmNMRData.ulElemNum;


    usLen             = sizeof(MTA_AT_QRY_NMR_CNF_STRU) + sizeof(RRC_MTA_GSM_NMR_ELEMENT_STRU) - 4;
    pstSndAtQryNmrCnf = (MTA_AT_QRY_NMR_CNF_STRU *)PS_MEM_ALLOC(UEPS_PID_MTA, usLen);

    if (VOS_NULL_PTR == pstSndAtQryNmrCnf)
    {
        MTA_ERROR_LOG("NAS_MMC_RcvGasQryNmrCnf: alloc msg fail!");

        stSndAtQryNmrCnf.enResult     = MTA_AT_RESULT_NO_ERROR;
        stSndAtQryNmrCnf.ucTotalIndex = ucFreqNum;
        stSndAtQryNmrCnf.ucCurrIndex  = 0;
        stSndAtQryNmrCnf.usNMRLen     = 0;
        PS_MEM_SET(stSndAtQryNmrCnf.aucNMRData, 0x0, sizeof(stSndAtQryNmrCnf.aucNMRData));

        TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                         ID_MTA_AT_QRY_NMR_CNF,
                         sizeof(stSndAtQryNmrCnf),
                         (VOS_UINT8*)&stSndAtQryNmrCnf);

        /* 从等待队列中删除消息 */
        TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);
        return;
    }

    for (ucLoop = 0; ucLoop < ucFreqNum; ucLoop++)
    {
        pstSndAtQryNmrCnf->enResult     = MTA_AT_RESULT_NO_ERROR;
        pstSndAtQryNmrCnf->ucTotalIndex = ucFreqNum;
        pstSndAtQryNmrCnf->ucCurrIndex  = ucLoop + 1;
        pstSndAtQryNmrCnf->usNMRLen     = sizeof(RRC_MTA_GSM_NMR_ELEMENT_STRU);

        PS_MEM_CPY(pstSndAtQryNmrCnf->aucNMRData, &pstRcvQryAsNmrCnf->u.stGsmNMRData.astNmrElem[ucLoop], pstSndAtQryNmrCnf->usNMRLen);

        TAF_MTA_SndAtMsg((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                         ID_MTA_AT_QRY_NMR_CNF,
                         usLen,
                         (VOS_UINT8*)pstSndAtQryNmrCnf);
    }

    PS_MEM_FREE(UEPS_PID_MTA, pstSndAtQryNmrCnf);

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);
    return;

}
VOS_VOID TAF_MTA_RcvRrcQryNmrCnf(VOS_VOID *pMsg)
{
    RRC_MTA_QRY_NMR_CNF_STRU           *pstRcvQryAsNmrCnf = VOS_NULL_PTR;
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf         = VOS_NULL_PTR;


    pstRcvQryAsNmrCnf = (RRC_MTA_QRY_NMR_CNF_STRU *)pMsg;

    /* 如果当前定时器不为运行状态 */
    if (TAF_MTA_TIMER_STATUS_RUNING != TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF))
    {
        MTA_WARNING_LOG("TAF_MTA_RcvRrcQryNmrCnf: WARNING: Timer was already stop!");
        return;
    }

    /* 停止保护定时器 */
    TAF_MTA_StopTimer(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);

    /* 获取当前定时器对应的消息队列 */
    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        return;
    }

    /* was nmr数据查询结果 */
    if (RRC_MTA_UTRAN_NMR == pstRcvQryAsNmrCnf->ulChoice)
    {
        NAS_MMC_RcvWasQryNmrCnf(pstRcvQryAsNmrCnf);
        return;
    }


    /* gas nmr数据查询结果 */
    if (RRC_MTA_GSM_NMR == pstRcvQryAsNmrCnf->ulChoice)
    {
        NAS_MMC_RcvGasQryNmrCnf(pstRcvQryAsNmrCnf);

    }

    return;
}
VOS_VOID TAF_MTA_RcvReselOffsetCfgSetReq(VOS_VOID *pMsg)
{
    AT_MTA_MSG_STRU                            *pstAtMtaMsg             = VOS_NULL_PTR;
    AT_MTA_RESEL_OFFSET_CFG_SET_NTF_STRU       *pstAtMtaReselOffsetSet  = VOS_NULL_PTR;
    MTA_RRC_RESEL_OFFSET_CFG_NTF_STRU          *pstMtaRrcReselOffsetCfg = VOS_NULL_PTR;
    VOS_UINT32                                  ulLength;

    pstAtMtaMsg            = (AT_MTA_MSG_STRU *)pMsg;

    pstAtMtaReselOffsetSet = (AT_MTA_RESEL_OFFSET_CFG_SET_NTF_STRU *)pstAtMtaMsg->aucContent;

    /* 申请消息结构内存 */
    ulLength                = sizeof(MTA_RRC_RESEL_OFFSET_CFG_NTF_STRU) - VOS_MSG_HEAD_LENGTH;
    pstMtaRrcReselOffsetCfg = (MTA_RRC_RESEL_OFFSET_CFG_NTF_STRU*)PS_ALLOC_MSG(UEPS_PID_MTA, ulLength);

    if (VOS_NULL_PTR == pstMtaRrcReselOffsetCfg)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvReselOffsetCfgSetReq: Alloc msg fail!");
        return ;
    }

    /* 构造消息结构体 */
    pstMtaRrcReselOffsetCfg->stMsgHeader.ulSenderCpuId   = VOS_LOCAL_CPUID;
    pstMtaRrcReselOffsetCfg->stMsgHeader.ulSenderPid     = UEPS_PID_MTA;
    pstMtaRrcReselOffsetCfg->stMsgHeader.ulReceiverCpuId = VOS_LOCAL_CPUID;
    pstMtaRrcReselOffsetCfg->stMsgHeader.ulReceiverPid   = WUEPS_PID_WRR;
    pstMtaRrcReselOffsetCfg->stMsgHeader.ulMsgName       = ID_MTA_RRC_RESEL_OFFSET_CFG_NTF;

    pstMtaRrcReselOffsetCfg->enOffsetEnable = pstAtMtaReselOffsetSet->ucOffsetFlg;

    /* 发送消息到AS */
    PS_SEND_MSG(UEPS_PID_MTA, pstMtaRrcReselOffsetCfg);

    return;
}
VOS_VOID TAF_MTA_RcvAtRefclkfreqSetReq(VOS_VOID *pMsg)
{
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg      = VOS_NULL_PTR;     /* 收到AT发来的消息指针 */
    AT_MTA_REFCLKFREQ_SET_REQ_STRU     *pstRefClkFreqReq    = VOS_NULL_PTR;
    TAF_MTA_REFCLOCK_INFO_STRU         *pstRefClockInfo     = VOS_NULL_PTR;
    MTA_AT_RESULT_ENUM_UINT32           enResult;

    pstAtMtaReqMsg      = (AT_MTA_MSG_STRU *)pMsg;
    pstRefClkFreqReq    = (AT_MTA_REFCLKFREQ_SET_REQ_STRU *)pstAtMtaReqMsg->aucContent;
    pstRefClockInfo     = TAF_MTA_GetRefClockInfo();

    if (pstRefClkFreqReq->enRptFlg >= AT_MTA_CMD_RPT_FLG_BUTT)
    {
        enResult = MTA_AT_RESULT_INCORRECT_PARAMETERS;
    }
    else
    {
        pstRefClockInfo->enRptFlg = pstRefClkFreqReq->enRptFlg;
        enResult = MTA_AT_RESULT_NO_ERROR;
    }

    TAF_MTA_SndAtRefclkfreqSetCnf(pstAtMtaReqMsg, enResult);

    return;
}
VOS_VOID TAF_MTA_SndAtRefclkfreqSetCnf(
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg,
    MTA_AT_RESULT_ENUM_UINT32           enResult
)
{
    MTA_AT_RESULT_CNF_STRU              stMtaAtCnf;

    /* 发送消息给AT模块 */
    stMtaAtCnf.enResult = enResult;

    TAF_MTA_SndAtMsg(&pstAtMtaReqMsg->stAppCtrl,
                     ID_MTA_AT_REFCLKFREQ_SET_CNF,
                     sizeof(MTA_AT_RESULT_CNF_STRU),
                     (VOS_UINT8*)&stMtaAtCnf);
    return;
}
VOS_VOID TAF_MTA_RcvAtRefclkfreqQryReq(VOS_VOID *pMsg)
{
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg      = VOS_NULL_PTR;     /* 收到AT发来的消息指针 */
    TAF_MTA_REFCLOCK_INFO_STRU         *pstRefClockInfo     = VOS_NULL_PTR;
    MTA_AT_REFCLKFREQ_QRY_CNF_STRU      stRefClkFreqCnf;

    PS_MEM_SET(&stRefClkFreqCnf, 0, sizeof(stRefClkFreqCnf));

    pstAtMtaReqMsg              = (AT_MTA_MSG_STRU *)pMsg;
    pstRefClockInfo             = TAF_MTA_GetRefClockInfo();
    stRefClkFreqCnf.enResult    = MTA_AT_RESULT_NO_ERROR;
    stRefClkFreqCnf.ulFreq      = pstRefClockInfo->ulFreq;
    stRefClkFreqCnf.ulPrecision = pstRefClockInfo->ulPrecision;
    stRefClkFreqCnf.enStatus    = pstRefClockInfo->enStatus;

    TAF_MTA_SndAtMsg(&pstAtMtaReqMsg->stAppCtrl,
                     ID_MTA_AT_REFCLKFREQ_QRY_CNF,
                     sizeof(MTA_AT_REFCLKFREQ_QRY_CNF_STRU),
                     (VOS_UINT8*)&stRefClkFreqCnf);

    return;
}
VOS_VOID TAF_MTA_SndAtRefclkfreqInd(VOS_VOID)
{
    TAF_MTA_REFCLOCK_INFO_STRU         *pstRefClockInfo     = VOS_NULL_PTR;
    MTA_AT_REFCLKFREQ_IND_STRU          stRefClkFreqInd;
    AT_APPCTRL_STRU                     stAppCtrl;

    pstRefClockInfo             = TAF_MTA_GetRefClockInfo();

    if (AT_MTA_CMD_RPT_FLG_OFF == pstRefClockInfo->enRptFlg)
    {
        /* 不允许主动上报时，不上报GPS参考时钟信息 */
        return;
    }

    PS_MEM_SET(&stRefClkFreqInd, 0, sizeof(stRefClkFreqInd));
    PS_MEM_SET(&stAppCtrl, 0, sizeof(stAppCtrl));
    stRefClkFreqInd.ulFreq      = pstRefClockInfo->ulFreq;
    stRefClkFreqInd.ulPrecision = pstRefClockInfo->ulPrecision;
    stRefClkFreqInd.enStatus    = pstRefClockInfo->enStatus;
    stAppCtrl.usClientId        = MTA_CLIENTID_BROADCAST;
    stAppCtrl.ucOpId            = MTA_INVALID_TAB_INDEX;

    TAF_MTA_SndAtMsg(&stAppCtrl,
                     ID_MTA_AT_REFCLKFREQ_IND,
                     sizeof(stRefClkFreqInd),
                     (VOS_UINT8*)&stRefClkFreqInd);

    return;
}
VOS_VOID TAF_MTA_RcvPhyRefClockStatusInd(VOS_VOID *pMsg)
{
    APM_MTA_REFCLOCK_STATUS_IND_STRU   *pstRefClockStatusInd    = VOS_NULL_PTR;
    TAF_MTA_REFCLOCK_INFO_STRU         *pstRefClockInfo         = VOS_NULL_PTR;

    pstRefClockStatusInd = (APM_MTA_REFCLOCK_STATUS_IND_STRU *)pMsg;
    pstRefClockInfo      = TAF_MTA_GetRefClockInfo();

    if ( (PHY_MTA_REFCLOCK_UNLOCKED == pstRefClockInfo->enStatus)
      && (PHY_MTA_REFCLOCK_UNLOCKED != pstRefClockStatusInd->enStatus) )
    {
        pstRefClockInfo->enStatus = PHY_MTA_REFCLOCK_LOCKED;
        TAF_MTA_SndAtRefclkfreqInd();
    }
    else if ( (PHY_MTA_REFCLOCK_UNLOCKED != pstRefClockInfo->enStatus)
           && (PHY_MTA_REFCLOCK_UNLOCKED == pstRefClockStatusInd->enStatus) )
    {
        pstRefClockInfo->enStatus = PHY_MTA_REFCLOCK_UNLOCKED;
        TAF_MTA_SndAtRefclkfreqInd();
    }
    else
    {
        /* 锁定状态没有变化，不处理 */
    }

    return;
}
VOS_VOID TAF_MTA_RcvAtEcidSetReq(VOS_VOID *pMsg)
{
    AT_MTA_MSG_STRU                    *pstAtMtaReqMsg      = VOS_NULL_PTR;     /* 收到AT发来的消息指针 */
    AT_MTA_ECID_SET_REQ_STRU           *pstEcidSetReq       = VOS_NULL_PTR;
    TAF_SDC_SYS_MODE_ENUM_UINT8         enSysmode;
    TAF_MTA_TIMER_STATUS_ENUM_UINT8     enQryNmrTiStatus;
    TAF_MTA_TIMER_STATUS_ENUM_UINT8     enEcidSetTiStatus;
    VOS_UINT32                          ulResult;

    /* 局部变量初始化 */
    pstAtMtaReqMsg  = (AT_MTA_MSG_STRU *)pMsg;
    pstEcidSetReq   = (AT_MTA_ECID_SET_REQ_STRU *)pstAtMtaReqMsg->aucContent;

    /* 判断命令版本号，当前仅支持0 */
    if (pstEcidSetReq->ulVersion != 0)
    {
        TAF_MTA_SndAtEcidSetCnf(&pstAtMtaReqMsg->stAppCtrl,
                                MTA_AT_RESULT_INCORRECT_PARAMETERS,
                                VOS_NULL_PTR);
        return;
    }

    /* 如果当前定时器已启动，则返回失败 */
    enQryNmrTiStatus  = TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF);
    enEcidSetTiStatus = TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);
    if ( (TAF_MTA_TIMER_STATUS_RUNING == enQryNmrTiStatus)
      || (TAF_MTA_TIMER_STATUS_RUNING == enEcidSetTiStatus) )
    {
        TAF_MTA_SndAtEcidSetCnf(&pstAtMtaReqMsg->stAppCtrl,
                                MTA_AT_RESULT_ERROR,
                                VOS_NULL_PTR);
        return;
    }

    /* 根据当前接入技术发送到对应的接入层查询小区信息 */
    enSysmode   = TAF_SDC_GetSysMode();
    ulResult    = VOS_ERR;
    switch (enSysmode)
    {
        case TAF_SDC_SYS_MODE_GSM:
            ulResult = TAF_MTA_SndAsQryNmrReqMsg(UEPS_PID_GAS);
            break;
        case TAF_SDC_SYS_MODE_WCDMA:
            if (NAS_UTRANCTRL_UTRAN_MODE_FDD == NAS_UTRANCTRL_GetCurrUtranMode())
            {
                ulResult = TAF_MTA_SndAsQryNmrReqMsg(WUEPS_PID_WRR);
            }
#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
            else
            {
                if (VOS_TRUE == TAF_MTA_ProcTdsEcidSetReq(&pstAtMtaReqMsg->stAppCtrl, &ulResult))
                {
                    /* 处理完成，无需后续处理，直接返回 */
                    return;
                }

            }
#endif
            break;
#if (FEATURE_ON == FEATURE_LTE)
        case TAF_SDC_SYS_MODE_LTE:
            if (VOS_FALSE == TAF_MTA_ProcEcidCustProgress(&ulResult))
            {
                /* 定制流程未处理，则发送查询请求给LRRC */
                ulResult = TAF_MTA_SndLrrcCellinfoQryReq();
            }

            break;
#endif
        default:
            break;
    }

    if (ulResult != VOS_OK)
    {
        /* 当前接入技术不支持(飞行模式)，或其他错误，返回NONE */
        TAF_MTA_SndAtEcidSetCnf(&pstAtMtaReqMsg->stAppCtrl,
                                MTA_AT_RESULT_NO_ERROR,
                                (VOS_UINT8*)MTA_ECID_NONE_RAT_STR);
        return;
    }

    /* 启动保护定时器 */
    TAF_MTA_StartTimer(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF, TI_TAF_MTA_WAIT_AS_ECID_SET_CNF_TIMER_LEN);

    /* 添加消息进等待队列 */
    TAF_MTA_SaveItemInCmdBufferQueue(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF,
                                     (VOS_UINT8*)&pstAtMtaReqMsg->stAppCtrl,
                                     sizeof(AT_APPCTRL_STRU));

    return;
}


VOS_VOID TAF_MTA_SndAtEcidSetCnf (
    AT_APPCTRL_STRU                    *pAppCtrl,
    MTA_AT_RESULT_ENUM_UINT32           enResult,
    VOS_UINT8                          *pucCellInfoStr
)
{
    MTA_AT_ECID_SET_CNF_STRU           *pstMtaAtEcidSetCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulLength;

    if (VOS_NULL_PTR == pucCellInfoStr)
    {
        ulLength = sizeof(MTA_AT_ECID_SET_CNF_STRU);
    }
    else
    {
        ulLength = sizeof(MTA_AT_ECID_SET_CNF_STRU) - 4 + VOS_StrLen((VOS_CHAR *)pucCellInfoStr) + 1;
    }

    pstMtaAtEcidSetCnf = (MTA_AT_ECID_SET_CNF_STRU *)PS_MEM_ALLOC(UEPS_PID_MTA, ulLength);

    if (VOS_NULL_PTR == pstMtaAtEcidSetCnf)
    {
        MTA_ERROR_LOG("TAF_MTA_SndAtEcidSetCnf: alloc msg fail!");
        return;
    }

    PS_MEM_SET((VOS_UINT8 *)pstMtaAtEcidSetCnf, 0, ulLength);

    /* 发送消息给AT模块 */
    pstMtaAtEcidSetCnf->enResult = enResult;

    if (VOS_NULL_PTR != pucCellInfoStr)
    {
        PS_MEM_CPY(pstMtaAtEcidSetCnf->aucCellInfoStr,
                   pucCellInfoStr,
                   VOS_StrLen((VOS_CHAR *)pucCellInfoStr));
    }

    TAF_MTA_SndAtMsg( pAppCtrl,
                      ID_MTA_AT_ECID_SET_CNF,
                      ulLength,
                      (VOS_UINT8*)pstMtaAtEcidSetCnf );

    PS_MEM_FREE(UEPS_PID_MTA, pstMtaAtEcidSetCnf);

    return;
}
VOS_VOID TAF_MTA_RcvGUAsEcellInfoQryCnf(VOS_VOID *pMsg)
{
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf         = VOS_NULL_PTR;
    RRC_MTA_QRY_NMR_CNF_STRU           *pstEcellInfoQryCnf = VOS_NULL_PTR;
    VOS_UINT8                          *pucCellInfoStr    = VOS_NULL_PTR;
    MTA_AT_RESULT_ENUM_UINT32           enResult;

    pstEcellInfoQryCnf = (RRC_MTA_QRY_NMR_CNF_STRU *)pMsg;

    /* 如果当前定时器不为运行状态 */
    if (TAF_MTA_TIMER_STATUS_RUNING != TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF))
    {
        TAF_MTA_RcvRrcQryNmrCnf(pMsg);
        return;
    }

    /* 停止保护定时器 */
    TAF_MTA_StopTimer(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);

    /* 获取当前定时器对应的消息队列 */
    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvGUAsEcellInfoQryCnf: No Buffer!");
        return;
    }

    pucCellInfoStr = (VOS_UINT8 *)PS_MEM_ALLOC(UEPS_PID_MTA, MTA_ECID_CELLINFO_MAX_STRLEN);
    if (VOS_NULL_PTR == pucCellInfoStr)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvGUAsEcellInfoQryCnf: Alloc msg fail!");
        return;
    }

    PS_MEM_SET(pucCellInfoStr, 0, MTA_ECID_CELLINFO_MAX_STRLEN);

    if (RRC_MTA_GSM_NMR == pstEcellInfoQryCnf->ulChoice)
    {
        enResult = TAF_MTA_CodeGasEcidStr(&(pstEcellInfoQryCnf->u.stGsmNMRData), pucCellInfoStr);
    }
    else if (RRC_MTA_UTRAN_NMR == pstEcellInfoQryCnf->ulChoice)
    {
        enResult = TAF_MTA_CodeWasEcidStr(&(pstEcellInfoQryCnf->u.stUtranNMRData), pucCellInfoStr);
    }
    else
    {
        enResult = MTA_AT_RESULT_ERROR;
    }

    if (MTA_AT_RESULT_NO_ERROR == enResult)
    {
        TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                                enResult,
                                pucCellInfoStr);
    }
    else
    {
        TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                                enResult,
                                VOS_NULL_PTR);
    }

    PS_MEM_FREE(UEPS_PID_MTA, pucCellInfoStr);

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;
}

/*lint -e438 -e830*/

VOS_VOID TAF_MTA_GetPlmnStr(
    VOS_UINT8                          *pucPlmnStr,
    TAF_SDC_PLMN_ID_STRU               *pstPlmnId
)
{
    VOS_INT32                           lMaxLength;
    VOS_INT32                           lLength;
    TAF_SDC_PLMN_ID_STRU                stPlmnIdTmp;

    PS_MEM_SET(&stPlmnIdTmp, 0, sizeof(stPlmnIdTmp));
    lMaxLength  = MTA_ECID_PLMN_MAX_STR_LEN;
    lLength     = 0;

    /* 将PLMN ID转换为BCD码 */
    stPlmnIdTmp.ulMcc = pstPlmnId->ulMcc;
    stPlmnIdTmp.ulMnc = pstPlmnId->ulMnc;
    TAF_SDC_PlmnId2Bcd(&stPlmnIdTmp);

    lLength = VOS_nsprintf((VOS_CHAR *)pucPlmnStr,
                            (VOS_UINT32)lMaxLength,
                            "%x%x%x,",
                            (stPlmnIdTmp.ulMcc & 0x0f00) >> 8,
                            (stPlmnIdTmp.ulMcc & 0xf0) >> 4,
                            (stPlmnIdTmp.ulMcc & 0x0f));

    if (lLength <= 0)
    {
        return;
    }

    lMaxLength -= lLength;

    if (0x0f00 == (stPlmnIdTmp.ulMnc & 0x0f00))
    {
        lLength = VOS_nsprintf((VOS_CHAR *)(pucPlmnStr + MTA_ECID_PLMN_MAX_STR_LEN - lMaxLength),
                                (VOS_UINT32)lMaxLength,
                               "%x%x",
                               (stPlmnIdTmp.ulMnc & 0xf0) >> 4,
                               (stPlmnIdTmp.ulMnc & 0x0f));
    }
    else
    {
        lLength = VOS_nsprintf((VOS_CHAR *)(pucPlmnStr + MTA_ECID_PLMN_MAX_STR_LEN - lMaxLength),
                                (VOS_UINT32)lMaxLength,
                                "%x%x%x",
                                (stPlmnIdTmp.ulMnc & 0x0f00) >> 8,
                                (stPlmnIdTmp.ulMnc & 0xf0) >> 4,
                                (stPlmnIdTmp.ulMnc & 0x0f));
    }

    return;
}
/*lint +e438 +e830*/

/*lint -e429 -e438 -e830*/

MTA_AT_RESULT_ENUM_UINT32 TAF_MTA_CodeGasEcidStr(
    RRC_MTA_GSM_NMR_STRU               *pstGsmNmr,
    VOS_UINT8                          *pucCellinfoStr
)
{
    TAF_SDC_CAMP_PLMN_INFO_STRU        *pstCampPlmnInfo     = VOS_NULL_PTR;
    VOS_INT32                           lMaxLength;
    VOS_INT32                           lLength;
    VOS_UINT32                          ulLoop;
    VOS_UINT8                           aucPlmnStr[MTA_ECID_PLMN_MAX_STR_LEN];

    lMaxLength     = MTA_ECID_CELLINFO_MAX_STRLEN;
    lLength        = 0;
    pstCampPlmnInfo = TAF_SDC_GetCampPlmnInfo();

    if (0 == pstGsmNmr->ulElemNum)
    {
        lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                                (VOS_UINT32)lMaxLength,
                                "%s",
                                MTA_ECID_NONE_RAT_STR);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        return MTA_AT_RESULT_NO_ERROR;
    }

    /* 0,GSM:,<MCC>,<MNC>,<LAC>,<CID>,<TA>,<NB_CELL> */
    PS_MEM_SET(aucPlmnStr, 0, sizeof(aucPlmnStr));
    TAF_MTA_GetPlmnStr(aucPlmnStr, &pstCampPlmnInfo->stPlmnId);
    lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                            (VOS_UINT32)lMaxLength,
                            "0,GSM:,%s,%d,%d,%d,%d",
                            aucPlmnStr,
                            pstCampPlmnInfo->usLac,
                            pstCampPlmnInfo->ulCellId,
                            MTA_ECID_UNDEFINED_VALUE,
                            pstGsmNmr->ulElemNum);

    if (lLength <= 0)
    {
        return MTA_AT_RESULT_INCORRECT_PARAMETERS;
    }

    lMaxLength -= lLength;

    for (ulLoop = 0; ulLoop < pstGsmNmr->ulElemNum; ulLoop++)
    {
        /* [,<BSIC>,<ARFCN>,<RX_level>]*NB_CELL */
        lLength = VOS_nsprintf((VOS_CHAR *)(pucCellinfoStr + MTA_ECID_CELLINFO_MAX_STRLEN - lMaxLength),
                                (VOS_UINT32)lMaxLength,
                                ",%d,%d,%d",
                                pstGsmNmr->astNmrElem[ulLoop].ucBsic,
                                pstGsmNmr->astNmrElem[ulLoop].usArfcn,
                                pstGsmNmr->astNmrElem[ulLoop].ucRxlev);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        lMaxLength -= lLength;
    }

    return MTA_AT_RESULT_NO_ERROR;
}
/*lint +e429 +e438 +e830*/

VOS_VOID TAF_MTA_GetUtranMeasInfo(
    NMR_UTRAN_MEASURED_RESULTS_STRU    *pstUtranMeasRslt,
    VOS_INT32                          *plUarfcnUL,
    VOS_INT32                          *plUarfcnDL,
    VOS_INT32                          *plUCRssi,
    VOS_UINT32                         *pulCMRNum)
{
    if (VOS_TRUE == pstUtranMeasRslt->bitOpFrequencyInfo)
    {
        if (NMR_UTRAN_MEASURED_TYPE_FDD == pstUtranMeasRslt->stFrequencyInfo.enFreqInfoChoice)
        {
            if (VOS_TRUE == pstUtranMeasRslt->stFrequencyInfo.u.stFreqInfoFDD.bitOpUarfcn_UL)
            {
                *plUarfcnUL = pstUtranMeasRslt->stFrequencyInfo.u.stFreqInfoFDD.usUlUarfcn;
            }
            *plUarfcnDL = pstUtranMeasRslt->stFrequencyInfo.u.stFreqInfoFDD.usDlUarfcn;
        }
    }

    if (VOS_TRUE == pstUtranMeasRslt->bitOpUTRA_CarrierRSSI)
    {
        *plUCRssi = (VOS_INT32)pstUtranMeasRslt->ulUTRA_CarrierRSSI;
    }

    if (VOS_TRUE == pstUtranMeasRslt->bitOpCellMeasResultsList)
    {
        *pulCMRNum = pstUtranMeasRslt->stCellMeasResultsList.ulMeasCellNum;
    }

    return;
}


VOS_VOID TAF_MTA_GetUtranCellInfo(
    NMR_UTRAN_CELL_MEAS_RESULTS_STRU   *pstCellMeasRslt,
    VOS_INT32                          *plCellId,
    VOS_INT32                          *plCellPSC,
    VOS_INT32                          *plEcNO,
    VOS_INT32                          *plRscp,
    VOS_INT32                          *plPathloss
)
{
    if (VOS_TRUE == pstCellMeasRslt->bitOpCellID)
    {
        *plCellId = (VOS_INT32)pstCellMeasRslt->ulCellID;
    }

    if (NMR_UTRAN_MEASURED_TYPE_FDD == pstCellMeasRslt->enCellMeasTypeChoice)
    {
        *plCellPSC = pstCellMeasRslt->u.stCellMeasResultsFDD.usCellPSC;

        if (VOS_TRUE == pstCellMeasRslt->u.stCellMeasResultsFDD.bitOpEcn0)
        {
            *plEcNO = pstCellMeasRslt->u.stCellMeasResultsFDD.ucEcn0;
        }

        if (VOS_TRUE == pstCellMeasRslt->u.stCellMeasResultsFDD.bitOpRscp)
        {
            *plRscp = pstCellMeasRslt->u.stCellMeasResultsFDD.ucRscp;
        }

        if (VOS_TRUE == pstCellMeasRslt->u.stCellMeasResultsFDD.bitOpPathloss)
        {
            *plPathloss = pstCellMeasRslt->u.stCellMeasResultsFDD.ucPathloss;
        }
    }

    return;
}


VOS_UINT32 TAF_MTA_GetUtranServingCellInfo(
    RRC_MTA_UTRAN_NMR_STRU             *pstUtranNmr,
    VOS_INT32                          *plCellPSC,
    VOS_INT32                          *plUarfcnUL,
    VOS_INT32                          *plUarfcnDL
)
{
    if ( (0 == pstUtranNmr->ulFreqNum)
      || (VOS_TRUE != pstUtranNmr->astMeasResults[0].bitOpCellMeasResultsList)
      || (0 == pstUtranNmr->astMeasResults[0].stCellMeasResultsList.ulMeasCellNum)
      || (NMR_UTRAN_MEASURED_TYPE_FDD != pstUtranNmr->astMeasResults[0]
                .stCellMeasResultsList.astCellMeasResults[0].enCellMeasTypeChoice) )
    {
        return VOS_ERR;
    }

    *plCellPSC = pstUtranNmr->astMeasResults[0].stCellMeasResultsList.astCellMeasResults[0].u.stCellMeasResultsFDD.usCellPSC;

    if (VOS_TRUE == pstUtranNmr->astMeasResults[0].bitOpFrequencyInfo)
    {
        if (NMR_UTRAN_MEASURED_TYPE_FDD != pstUtranNmr->astMeasResults[0].stFrequencyInfo.enFreqInfoChoice)
        {
            return VOS_ERR;
        }

        if (VOS_TRUE == pstUtranNmr->astMeasResults[0].stFrequencyInfo.u.stFreqInfoFDD.bitOpUarfcn_UL)
        {
            *plUarfcnUL = pstUtranNmr->astMeasResults[0].stFrequencyInfo.u.stFreqInfoFDD.usUlUarfcn;
        }

        *plUarfcnDL = pstUtranNmr->astMeasResults[0].stFrequencyInfo.u.stFreqInfoFDD.usDlUarfcn;
    }

    return VOS_OK;
}

/*lint -e429 -e830*/

MTA_AT_RESULT_ENUM_UINT32 TAF_MTA_CodeWasEcidStr(
    RRC_MTA_UTRAN_NMR_STRU             *pstUtranNmr,
    VOS_UINT8                          *pucCellinfoStr
)
{
    TAF_SDC_CAMP_PLMN_INFO_STRU        *pstCampPlmnInfo     = VOS_NULL_PTR;
    VOS_UINT8                           aucPlmnStr[MTA_ECID_PLMN_MAX_STR_LEN];
    VOS_INT32                           lMaxLength;
    VOS_INT32                           lLength;
    VOS_UINT32                          ulLoop1;
    VOS_UINT32                          ulLoop2;
    VOS_INT32                           lUarfcnUL;
    VOS_INT32                           lUarfcnDL;
    VOS_INT32                           lUCRssi;
    VOS_UINT32                          ulCMRNum;
    VOS_INT32                           lCellId;
    VOS_INT32                           lCellPSC;
    VOS_INT32                           lEcNO;
    VOS_INT32                           lRscp;
    VOS_INT32                           lPathloss;

    lMaxLength      = MTA_ECID_CELLINFO_MAX_STRLEN;
    lLength         = 0;
    pstCampPlmnInfo = TAF_SDC_GetCampPlmnInfo();
    lUarfcnUL       = MTA_ECID_UNDEFINED_VALUE;
    lUarfcnDL       = MTA_ECID_UNDEFINED_VALUE;
    lCellPSC        = MTA_ECID_UNDEFINED_VALUE;

    if (VOS_OK != TAF_MTA_GetUtranServingCellInfo(pstUtranNmr, &lCellPSC, &lUarfcnUL, &lUarfcnDL))
    {
        lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                                (VOS_UINT32)lMaxLength,
                                "%s",
                                MTA_ECID_NONE_RAT_STR);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        return MTA_AT_RESULT_NO_ERROR;
    }

    /* 0,WCDMA:,<MCC>,<MNC>,<UCID>,<Primary_Scrambling>,<UARFCN_UL>,<UARFCN_DL>,<NB_FREQ> */
    PS_MEM_SET(aucPlmnStr, 0, sizeof(aucPlmnStr));
    TAF_MTA_GetPlmnStr(aucPlmnStr, &pstCampPlmnInfo->stPlmnId);
    lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                           (VOS_UINT32)lMaxLength,
                            "0,WCDMA:,%s,%d,%d,%d,%d,%d",
                            aucPlmnStr,
                            pstCampPlmnInfo->ulCellId,
                            lCellPSC,
                            lUarfcnUL,
                            lUarfcnDL,
                            pstUtranNmr->ulFreqNum);

    if (lLength <= 0)
    {
        return MTA_AT_RESULT_INCORRECT_PARAMETERS;
    }

    lMaxLength -= lLength;

    for (ulLoop1 = 0; ulLoop1 < pstUtranNmr->ulFreqNum; ulLoop1++)
    {
        /* [,<UARFCN_UL>,<UARFCN_DL>,<UC_RSSI>,<CMR_NB>
            [,<Cell_ID>,<Primary_CPICH>,<CPICH_Ec_NO>,<CPICH_RSCP>,<Pathloss>]*CMR_NB]*NB_FREQ */
        lUarfcnUL   = MTA_ECID_UNDEFINED_VALUE;
        lUarfcnDL   = MTA_ECID_UNDEFINED_VALUE;
        lUCRssi     = MTA_ECID_UNDEFINED_VALUE;
        ulCMRNum    = 0;

        TAF_MTA_GetUtranMeasInfo(&pstUtranNmr->astMeasResults[ulLoop1],
                                 &lUarfcnUL, &lUarfcnDL, &lUCRssi, &ulCMRNum);

        lLength = VOS_nsprintf((VOS_CHAR *)(pucCellinfoStr + MTA_ECID_CELLINFO_MAX_STRLEN - lMaxLength),
                               (VOS_UINT32)lMaxLength,
                                ",%d,%d,%d,%d",
                                lUarfcnUL,
                                lUarfcnDL,
                                lUCRssi,
                                ulCMRNum);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        lMaxLength -= lLength;

        for (ulLoop2 = 0; ulLoop2 < ulCMRNum; ulLoop2++)
        {
            lCellId     = MTA_ECID_UNDEFINED_VALUE;
            lCellPSC    = MTA_ECID_UNDEFINED_VALUE;
            lEcNO       = MTA_ECID_UNDEFINED_VALUE;
            lRscp       = MTA_ECID_RSCP_UNDEFINED_VALUE;
            lPathloss   = MTA_ECID_UNDEFINED_VALUE;

            TAF_MTA_GetUtranCellInfo(&pstUtranNmr->astMeasResults[ulLoop1].stCellMeasResultsList.astCellMeasResults[ulLoop2],
                                     &lCellId, &lCellPSC, &lEcNO, &lRscp, &lPathloss);

            lLength = VOS_nsprintf((VOS_CHAR *)(pucCellinfoStr + MTA_ECID_CELLINFO_MAX_STRLEN - lMaxLength),
                                   (VOS_UINT32)lMaxLength,
                                    ",%d,%d,%d,%d,%d",
                                    lCellId,
                                    lCellPSC,
                                    lEcNO,
                                    lRscp,
                                    lPathloss);

            if (lLength <= 0)
            {
                return MTA_AT_RESULT_INCORRECT_PARAMETERS;
            }

            lMaxLength -= lLength;

        }
    }

    return MTA_AT_RESULT_NO_ERROR;
}
/*lint +e429 +e830*/

#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
VOS_UINT32 TAF_MTA_ProcTdsEcidSetReq(
    AT_APPCTRL_STRU                    *pstAppCtrl,
    VOS_UINT32                         *pulResult
)
{
    TAF_SDC_CAMP_PLMN_INFO_STRU        *pstCampPlmnInfo     = VOS_NULL_PTR;
    VOS_UINT8                          *pucCellInfoStr      = VOS_NULL_PTR;
    VOS_INT32                           lLength;
    VOS_UINT8                           aucPlmnStr[MTA_ECID_PLMN_MAX_STR_LEN];

    if (VOS_TRUE == TAF_MTA_ProcEcidCustProgress(pulResult))
    {
        /* 定制流程处理完成，则交由后续处理 */
        return VOS_FALSE;
    }

    pucCellInfoStr = (VOS_UINT8 *)PS_MEM_ALLOC(UEPS_PID_MTA, MTA_ECID_CELLINFO_MAX_STRLEN);
    if (VOS_NULL_PTR == pucCellInfoStr)
    {
        MTA_ERROR_LOG("TAF_MTA_ProcTdsEcidSetReq: Alloc fail!");
        *pulResult = VOS_ERR;
        return VOS_FALSE;
    }

    PS_MEM_SET(pucCellInfoStr, 0, MTA_ECID_CELLINFO_MAX_STRLEN);
    lLength         = 0;
    pstCampPlmnInfo = TAF_SDC_GetCampPlmnInfo();

    /* 0,GSM:,<MCC>,<MNC>,<LAC>,<CID>,-1,0 */
    PS_MEM_SET(aucPlmnStr, 0, sizeof(aucPlmnStr));
    TAF_MTA_GetPlmnStr(aucPlmnStr, &pstCampPlmnInfo->stPlmnId);
    lLength = VOS_nsprintf((VOS_CHAR *)pucCellInfoStr,
                            (VOS_UINT32)MTA_ECID_CELLINFO_MAX_STRLEN,
                            "0,GSM:,%s,%d,%d,%d,0",
                            aucPlmnStr,
                            pstCampPlmnInfo->usLac,
                            pstCampPlmnInfo->ulCellId,
                            MTA_ECID_UNDEFINED_VALUE);

    if (lLength <= 0)
    {
        TAF_MTA_SndAtEcidSetCnf(pstAppCtrl,
                                MTA_AT_RESULT_INCORRECT_PARAMETERS,
                                VOS_NULL_PTR);
    }
    else
    {
        TAF_MTA_SndAtEcidSetCnf(pstAppCtrl,
                                MTA_AT_RESULT_NO_ERROR,
                                pucCellInfoStr);
    }

    PS_MEM_FREE(UEPS_PID_MTA, pucCellInfoStr);

    *pulResult = VOS_OK;
    return VOS_TRUE;
}
#endif

#if (FEATURE_ON == FEATURE_LTE)
VOS_UINT32 TAF_MTA_SndLrrcCellinfoQryReq(VOS_VOID)
{
    VOS_UINT32                          ulLength;
    MTA_LRRC_CELLINFO_QRY_REQ_STRU     *pstCellinfoQryReq = VOS_NULL_PTR;

    /* 申请消息结构内存 */
    ulLength              = sizeof(MTA_LRRC_CELLINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH;
    pstCellinfoQryReq     = (MTA_LRRC_CELLINFO_QRY_REQ_STRU *)PS_ALLOC_MSG(UEPS_PID_MTA, ulLength);
    if (VOS_NULL_PTR == pstCellinfoQryReq)
    {
        MTA_ERROR_LOG("TAF_MTA_SndLrrcCellinfoQryReq: Alloc msg fail!");
        return VOS_ERR;
    }

    /* 构造消息结构体 */
    pstCellinfoQryReq->stMsgHeader.ulSenderCpuId     = VOS_LOCAL_CPUID;
    pstCellinfoQryReq->stMsgHeader.ulSenderPid       = UEPS_PID_MTA;
    pstCellinfoQryReq->stMsgHeader.ulReceiverCpuId   = VOS_LOCAL_CPUID;
    pstCellinfoQryReq->stMsgHeader.ulReceiverPid     = PS_PID_ERRC;
    pstCellinfoQryReq->stMsgHeader.ulMsgName         = ID_MTA_LRRC_CELLINFO_QRY_REQ;

    PS_MEM_SET(pstCellinfoQryReq->aucReserved, 0, sizeof(pstCellinfoQryReq->aucReserved));

    /* 发送消息到LTE */
    if (VOS_OK != PS_SEND_MSG(UEPS_PID_MTA, pstCellinfoQryReq))
    {
        MTA_ERROR_LOG("TAF_MTA_SndLrrcCellinfoQryReq(): Send MSG Fail!");
    }

    return VOS_OK;
}
VOS_VOID TAF_MTA_RcvLrrcCellinfoQryCnf(VOS_VOID *pMsg)
{
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf         = VOS_NULL_PTR;
    LRRC_MTA_CELLINFO_QRY_CNF_STRU     *pstCellinfoQryCnf = VOS_NULL_PTR;
    VOS_UINT8                          *pucCellInfoStr    = VOS_NULL_PTR;
    MTA_AT_RESULT_ENUM_UINT32           enResult;

    pstCellinfoQryCnf = (LRRC_MTA_CELLINFO_QRY_CNF_STRU *)pMsg;
    enResult          = MTA_AT_RESULT_ERROR;

    /* 如果当前定时器不为运行状态 */
    if (TAF_MTA_TIMER_STATUS_RUNING != TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF))
    {
        MTA_WARNING_LOG("TAF_MTA_RcvLrrcCellinfoQryCnf: Timer was already stoped!");
        return;
    }

    /* 停止保护定时器 */
    TAF_MTA_StopTimer(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);

    /* 获取当前定时器对应的消息队列 */
    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvLrrcCellinfoQryCnf: No Buffer!");
        return;
    }

    pucCellInfoStr = (VOS_UINT8 *)PS_MEM_ALLOC(UEPS_PID_MTA, MTA_ECID_CELLINFO_MAX_STRLEN);
    if (VOS_NULL_PTR == pucCellInfoStr)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvLrrcCellinfoQryCnf: Alloc msg fail!");
        return;
    }

    PS_MEM_SET(pucCellInfoStr, 0, MTA_ECID_CELLINFO_MAX_STRLEN);

    if (MTA_RRC_RESULT_NO_ERROR == pstCellinfoQryCnf->enResult)
    {
        enResult = TAF_MTA_CodeLteEcidStr(&(pstCellinfoQryCnf->stCellInfoRslt), pucCellInfoStr);
    }

    if (MTA_AT_RESULT_NO_ERROR == enResult)
    {
        TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                                enResult,
                                pucCellInfoStr);
    }
    else
    {
        TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                                enResult,
                                VOS_NULL_PTR);
    }

    PS_MEM_FREE(UEPS_PID_MTA, pucCellInfoStr);

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;
}


VOS_VOID TAF_MTA_GetLrrcCellInfo(
    LRRC_CELLINFO_STRU                 *pstCellInfo,
    VOS_INT32                          *plEarfcn,
    VOS_INT32                          *plRsrp,
    VOS_INT32                          *plRsrq,
    VOS_INT32                          *plTa
)
{
    if (VOS_TRUE == pstCellInfo->bitOpEarfcn)
    {
        *plEarfcn = (VOS_INT32)pstCellInfo->ulEarfcn;
    }

    if (VOS_TRUE == pstCellInfo->bitOpRsrp)
    {
        *plRsrp = (VOS_INT32)pstCellInfo->ulRsrp;
    }

    if (VOS_TRUE == pstCellInfo->bitOpRsrq)
    {
        *plRsrq = (VOS_INT32)pstCellInfo->ulRsrq;
    }

    if (VOS_TRUE == pstCellInfo->bitOpTa)
    {
        *plTa= (VOS_INT32)(pstCellInfo->ulTa >> MTA_ECID_LRRC_TA_TRANSPARA);
    }

    return;
}

/*lint -e429 -e830*/

MTA_AT_RESULT_ENUM_UINT32 TAF_MTA_CodeLteEcidStr(
    LRRC_CELLINFO_RSLT_STRU            *pstCellinfoRslt,
    VOS_UINT8                          *pucCellinfoStr
)
{
    TAF_SDC_CAMP_PLMN_INFO_STRU        *pstCampPlmnInfo     = VOS_NULL_PTR;
    VOS_UINT8                           aucPlmnStr[MTA_ECID_PLMN_MAX_STR_LEN];
    VOS_INT32                           lMaxLength;
    VOS_INT32                           lLength;
    VOS_UINT32                          ulLoop;
    VOS_INT32                           lPhyCellCode;
    VOS_INT32                           lEarfcn;
    VOS_INT32                           lRsrp;
    VOS_INT32                           lRsrq;
    VOS_INT32                           lTa;

    lMaxLength     = MTA_ECID_CELLINFO_MAX_STRLEN;
    lLength        = 0;
    pstCampPlmnInfo = TAF_SDC_GetCampPlmnInfo();

    if (0 == pstCellinfoRslt->ulCellNum)
    {
        lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                               (VOS_UINT32)lMaxLength,
                                "%s",
                                MTA_ECID_NONE_RAT_STR);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        return MTA_AT_RESULT_NO_ERROR;
    }

    /* 0,LTE:,<MCC>,<MNC>,<TAC>,<LCID>,<L_NB_CELL> */
    PS_MEM_SET(aucPlmnStr, 0, sizeof(aucPlmnStr));
    TAF_MTA_GetPlmnStr(aucPlmnStr, &pstCampPlmnInfo->stPlmnId);
    lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                           (VOS_UINT32)lMaxLength,
                            "0,LTE:,%s,%d,%d,%d",
                            aucPlmnStr,
                            pstCellinfoRslt->ulTac,
                            pstCampPlmnInfo->ulCellId,
                            pstCellinfoRslt->ulCellNum);

    if (lLength <= 0)
    {
        return MTA_AT_RESULT_INCORRECT_PARAMETERS;
    }

    lMaxLength -= lLength;

    for (ulLoop = 0; ulLoop < pstCellinfoRslt->ulCellNum; ulLoop++)
    {
        lPhyCellCode    = (VOS_INT32)pstCellinfoRslt->astCellInfo[ulLoop].ulPhyCellCode;
        lEarfcn         = MTA_ECID_UNDEFINED_VALUE;
        lRsrp           = MTA_ECID_UNDEFINED_VALUE;
        lRsrq           = MTA_ECID_UNDEFINED_VALUE;
        lTa             = MTA_ECID_UNDEFINED_VALUE;

        /* [,<Cell_Code>,<EARFCN>,<RSRP>,<RSRQ>,<L_TA>] *NB_CELL */
        TAF_MTA_GetLrrcCellInfo(&pstCellinfoRslt->astCellInfo[ulLoop],
                                &lEarfcn, &lRsrp, &lRsrq, &lTa);

        lLength = VOS_nsprintf((VOS_CHAR *)(pucCellinfoStr + MTA_ECID_CELLINFO_MAX_STRLEN - lMaxLength),
                               (VOS_UINT32)lMaxLength,
                                ",%d,%d,%d,%d,%d",
                                lPhyCellCode,
                                lEarfcn,
                                lRsrp,
                                lRsrq,
                                lTa);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        lMaxLength -= lLength;

    }

    return MTA_AT_RESULT_NO_ERROR;
}
/*lint +e429 +e830*/
#endif
VOS_VOID TAF_MTA_RcvTiEcidSetExpired(VOS_VOID *pMsg)
{
    TAF_MTA_CMD_BUFFER_STRU                *pstCmdBuf;

    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);

    if (VOS_NULL_PTR == pstCmdBuf)
    {
        MTA_WARNING_LOG("TAF_MTA_RcvTiEcidSetExpired: WARNING: buffer is null!");
        return;
    }

    /* 发送消息给AT模块 */
    TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                            MTA_AT_RESULT_OPTION_TIMEOUT,
                            VOS_NULL_PTR);

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;

}


VOS_UINT32 TAF_MTA_ProcEcidCustProgress(
    VOS_UINT32                         *pulResult
)
{
    TAF_SDC_CAMP_PLMN_INFO_STRU        *pstCampPlmnInfo     = VOS_NULL_PTR;
    TAF_NVIM_ECID_TL2GSM_CFG_STRU      *pstEcidCfg          = VOS_NULL_PTR;
    VOS_UINT32                          ulLength;
    MTA_MTC_RESERVE_STRU               *pstReserveMsg       = VOS_NULL_PTR;
    TAF_SDC_PLMN_ID_STRU                stPlmnId;

    pstEcidCfg      = TAF_MTA_GetEcidCfg();
    pstCampPlmnInfo = TAF_SDC_GetCampPlmnInfo();
    stPlmnId        = pstCampPlmnInfo->stPlmnId;
    TAF_SDC_PlmnId2Bcd(&stPlmnId);

    if ( (VOS_TRUE == pstEcidCfg->ucEnableFlg)
      && (pstEcidCfg->usMcc == stPlmnId.ulMcc) )
    {
        /* 申请消息结构内存 */
        ulLength        = sizeof(MTA_MTC_RESERVE_STRU) - VOS_MSG_HEAD_LENGTH;
        pstReserveMsg   = (MTA_MTC_RESERVE_STRU *)PS_ALLOC_MSG(UEPS_PID_MTA, ulLength);
        if (VOS_NULL_PTR == pstReserveMsg)
        {
            MTA_ERROR_LOG("TAF_MTA_ProcEcidCustProgress(): Alloc msg fail!");
            *pulResult = VOS_ERR;
            return VOS_FALSE;
        }

        /* 构造消息结构体 */
        pstReserveMsg->stMsgHeader.ulReceiverPid     = UEPS_PID_MTC;
        pstReserveMsg->stMsgHeader.ulMsgName         = ID_MTA_MTC_GSM_CELLINFO_QRY_REQ;

        PS_MEM_SET(pstReserveMsg->aucReserved, 0, sizeof(pstReserveMsg->aucReserved));

        *pulResult = VOS_OK;

        /* 发送消息到MTC */
        if (VOS_OK != PS_SEND_MSG(UEPS_PID_MTA, pstReserveMsg))
        {
            MTA_ERROR_LOG("TAF_MTA_ProcEcidCustProgress(): Send MSG Fail!");
            *pulResult = VOS_ERR;
        }

        return VOS_TRUE;
    }

    *pulResult = VOS_OK;
    return VOS_FALSE;
}

/*lint -e429 -e438 -e830*/

MTA_AT_RESULT_ENUM_UINT32 TAF_MTA_CodeGasEcidStrEx(
    MTC_GSM_CELLINFO_EX_STRU           *pstGsmCellInfoEx,
    VOS_UINT8                          *pucCellinfoStr
)
{
    TAF_SDC_PLMN_ID_STRU                stPlmnId;
    VOS_INT32                           lLength;
    VOS_UINT8                           aucPlmnStr[MTA_ECID_PLMN_MAX_STR_LEN];

    /* 如果GSM小区信息无效，则直接给AT返回空结果 */
    if (VOS_FALSE == pstGsmCellInfoEx->ulValidFlg)
    {
        lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                                MTA_ECID_CELLINFO_MAX_STRLEN,
                                "%s",
                                MTA_ECID_NONE_RAT_STR);

        if (lLength <= 0)
        {
            return MTA_AT_RESULT_INCORRECT_PARAMETERS;
        }

        return MTA_AT_RESULT_NO_ERROR;
    }

    stPlmnId.ulMcc = pstGsmCellInfoEx->stPlmnId.ulMcc;
    stPlmnId.ulMnc = pstGsmCellInfoEx->stPlmnId.ulMnc;

    /* 0,GSM:,<MCC>,<MNC>,<LAC>,<CID>,<TA>,<NB_CELL> */
    PS_MEM_SET(aucPlmnStr, 0, sizeof(aucPlmnStr));
    TAF_MTA_GetPlmnStr(aucPlmnStr, &stPlmnId);
    lLength = VOS_nsprintf((VOS_CHAR *)pucCellinfoStr,
                            MTA_ECID_CELLINFO_MAX_STRLEN,
                            "0,GSM:,%s,%d,%d,%d,%d",
                            aucPlmnStr,
                            pstGsmCellInfoEx->usLac,
                            pstGsmCellInfoEx->usCellId,
                            MTA_ECID_UNDEFINED_VALUE,
                            0);

    if (lLength <= 0)
    {
        return MTA_AT_RESULT_INCORRECT_PARAMETERS;
    }

    return MTA_AT_RESULT_NO_ERROR;
}
/*lint +e429 +e438 +e830*/

VOS_VOID TAF_MTA_RcvMtcGsmCellInfoQryCnf(VOS_VOID *pMsg)
{
    TAF_MTA_CMD_BUFFER_STRU            *pstCmdBuf               = VOS_NULL_PTR;
    MTC_MTA_GSM_CELLINFO_QRY_CNF_STRU  *pstGsmCellInfoQryCnf    = VOS_NULL_PTR;
    VOS_UINT8                          *pucCellInfoStr          = VOS_NULL_PTR;
    MTA_AT_RESULT_ENUM_UINT32           enResult;

    pstGsmCellInfoQryCnf    = (MTC_MTA_GSM_CELLINFO_QRY_CNF_STRU *)pMsg;
    enResult                = MTA_AT_RESULT_ERROR;

    /* 如果当前定时器不为运行状态 */
    if (TAF_MTA_TIMER_STATUS_RUNING != TAF_MTA_GetTimerStatus(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF))
    {
        MTA_WARNING_LOG("TAF_MTA_RcvMtcGsmCellInfoQryCnf: Timer was already stoped!");
        return;
    }

    /* 停止保护定时器 */
    TAF_MTA_StopTimer(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);

    /* 获取当前定时器对应的消息队列 */
    pstCmdBuf = TAF_MTA_GetItemFromCmdBufferQueue(TI_TAF_MTA_WAIT_AS_ECID_SET_CNF);
    if (VOS_NULL_PTR == pstCmdBuf)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvMtcGsmCellInfoQryCnf: No Buffer!");
        return;
    }

    pucCellInfoStr = (VOS_UINT8 *)PS_MEM_ALLOC(UEPS_PID_MTA, MTA_ECID_CELLINFO_MAX_STRLEN);
    if (VOS_NULL_PTR == pucCellInfoStr)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvMtcGsmCellInfoQryCnf: Alloc msg fail!");
        return;
    }

    PS_MEM_SET(pucCellInfoStr, 0, MTA_ECID_CELLINFO_MAX_STRLEN);

    if (MTA_MTC_RESULT_NO_ERROR == pstGsmCellInfoQryCnf->enResult)
    {
        enResult = TAF_MTA_CodeGasEcidStrEx(&(pstGsmCellInfoQryCnf->stCellInfoEx), pucCellInfoStr);
    }

    if (MTA_AT_RESULT_NO_ERROR == enResult)
    {
        TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                                enResult,
                                pucCellInfoStr);
    }
    else
    {
        TAF_MTA_SndAtEcidSetCnf((AT_APPCTRL_STRU *)pstCmdBuf->pucMsgInfo,
                                enResult,
                                VOS_NULL_PTR);
    }

    PS_MEM_FREE(UEPS_PID_MTA, pucCellInfoStr);

    /* 从等待队列中删除消息 */
    TAF_MTA_DelItemInCmdBufferQueue(pstCmdBuf->enTimerId);

    return;
}


/*lint +e958*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

