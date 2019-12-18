#ifndef __ORTOOL_DTWMANAGER_TOOL_H__
#define __ORTOOL_DTWMANAGER_TOOL_H__
/***************************************************************************************
 Autodesk(R) Open Reality(R) Samples
 
 (C) 2009 Autodesk, Inc. and/or its licensors
 All rights reserved.
 
 AUTODESK SOFTWARE LICENSE AGREEMENT
 Autodesk, Inc. licenses this Software to you only upon the condition that 
 you accept all of the terms contained in the Software License Agreement ("Agreement") 
 that is embedded in or that is delivered with this Software. By selecting 
 the "I ACCEPT" button at the end of the Agreement or by copying, installing, 
 uploading, accessing or using all or any portion of the Software you agree 
 to enter into the Agreement. A contract is then formed between Autodesk and 
 either you personally, if you acquire the Software for yourself, or the company 
 or other legal entity for which you are acquiring the software.
 
 AUTODESK, INC., MAKES NO WARRANTY, EITHER EXPRESS OR IMPLIED, INCLUDING BUT 
 NOT LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
 PURPOSE REGARDING THESE MATERIALS, AND MAKES SUCH MATERIALS AVAILABLE SOLELY ON AN 
 "AS-IS" BASIS.
 
 IN NO EVENT SHALL AUTODESK, INC., BE LIABLE TO ANYONE FOR SPECIAL, COLLATERAL, 
 INCIDENTAL, OR CONSEQUENTIAL DAMAGES IN CONNECTION WITH OR ARISING OUT OF PURCHASE 
 OR USE OF THESE MATERIALS. THE SOLE AND EXCLUSIVE LIABILITY TO AUTODESK, INC., 
 REGARDLESS OF THE FORM OF ACTION, SHALL NOT EXCEED THE PURCHASE PRICE OF THE 
 MATERIALS DESCRIBED HEREIN.
 
 Autodesk, Inc., reserves the right to revise and improve its products as it sees fit.
 
 Autodesk and Open Reality are registered trademarks or trademarks of Autodesk, Inc., 
 in the U.S.A. and/or other countries. All other brand names, product names, or 
 trademarks belong to their respective holders. 
 
 GOVERNMENT USE
 Use, duplication, or disclosure by the U.S. Government is subject to restrictions as 
 set forth in FAR 12.212 (Commercial Computer Software-Restricted Rights) and 
 DFAR 227.7202 (Rights in Technical Data and Computer Software), as applicable. 
 Manufacturer is Autodesk, Inc., 10 Duke Street, Montreal, Quebec, Canada, H3C 2L7.
***************************************************************************************/

/**	\file	ortoolconstraint_tool.h
*	Declaration for a media import tool.
*	Media import tool (with extras) class declaration (FBMediaImportTool).
*/

//--- SDK include
#include <fbsdk/fbsdk.h>
#include <string>
#include <vector>
#include "pugixml.hpp"

using namespace std;

//--- Registration define
#define ORTOOLDTWMANAGER__CLASSNAME	ORToolDTWManager
#define ORTOOLDTWMANAGER__CLASSSTR	"ORToolDTWManager"

// define structure for storing randomised warps
struct RandomWarp
{
	int effector;
	int warpType;
	int warpValue;
};

/**	Tool template.
*/
class ORToolDTWManager : public FBTool
{
	//--- FiLMBOX Tool declaration.
	FBToolDeclare(ORToolDTWManager, FBTool);

	
public:
	//--- FiLMBOX Construction/Destruction,
	virtual bool FBCreate();		//!< FiLMBOX Creation function.
	virtual void FBDestroy();		//!< FiLMBOX Destruction function.

	void UICreate	();
	void UIConfigure();
	void UIReset	();
	void UIRefresh	();
	
	// general functions
	void SetProp(FBModel* pTopModel, const char* pName, double pValue);
	void SetProp(FBModel* pModel, const char* pName, FBColor* pValue);

	void SetPropForAllNodes(FBModel* pModel, const char* pName, double pValue);
	void SetPropForAllNodes(FBModel* pModel, const char* pName, FBColor* pValue);

	void ListProperties(FBPropertyManager pPropManager);
	void SelectBranch(FBModel* pTopmodel);
	void setModelColour(FBModel*, double pColour[]);
	
	// import functions
	FBCharacter* ImportMotion(const char* pFilename, vector<string> &pListOfNodesToMap, const char* pName);
	void labelCharacter(FBCharacter* pCharacter, const char* pName);
	void ConnectCharacterToMotion(FBCharacter* lCharacter, FBModel* pTopModel);
	void MapObjectToCharacter(FBCharacter* lCharacetr, FBModel* pObject);

	// warp functions
	void addAllNodesToWarp(FBModel* pModel, FBAnimationNode* pTimeWarp);
	void StretchWarp(FBFCurve* pCurve, double pValue);
	void ShrinkWarp(FBFCurve* pCurve, double pValue);
	void FastSlowWarp(FBFCurve* pCurve, double pValue);
	void SlowFastWarp(FBFCurve* pCurve, double pValue);
	void SlowInSlowOutWarp(FBFCurve* pCurve, double pValue);
	void FastInFastOutWarp(FBFCurve* pCurve, double pValue);
	const char* EffectorLookup(int pEffectorID);
	void ORToolDTWManager::PopulateRandomWarpDisp();
	void ORToolDTWManager::SetFirstKeyToCubic(FBFCurve* pCurve);

	// DTW functions
	void SetUpAnimatonNodes(void);
	FBVector3d EvaluateNodes(FBAnimationNode* pAnimNode, FBTime pTime);
	FBVector3d lPredictPos(FBVector3d pPrevFramePos, FBVector3d pCurrentFramePos);
	void add3DVectorKey(FBAnimationNode* pAnimNode, FBVector3d pVector, FBTime pTime);
	double calcDistance3d(FBVector3d pV1, FBVector3d pV2);
	FBVector3d add3dVect(FBVector3d pV1, FBVector3d pV2);
	FBTime FindLastKey(FBCharacter* pCharacter);
	
	// function callbacks for buttons
private:
	void	EventImportMotionClick(HISender pSender, HKEvent pEvent);

	void	EventButtonPerformWarp(HISender pSender, HKEvent pEvent);

	void    EventButtonGenerateRandomWarp(HISender pSender, HKEvent pEvent);
	void	EventButtonSaveRandomWarp(HISender pSender, HKEvent pEvent);
	void    EventButtonOpenRandomWarp(HISender pSender, HKEvent pEvent);
	void    EventButtonApplyRandomWarp(HISender pSender, HKEvent pEvent);

	void	EventButtonRunPredictiveDTW(HISender pSender, HKEvent pEvent);
	void    EventButtonRunStandardDTW(HISender pSender, HKEvent pEvent);

	void    EventButtonExportCurves(HISender pSender, HKEvent pEvent);

	void	EventIdle(HISender pSender, HKEvent pEvent);
	void	EventHandler(HISender pSender, HKEvent pEvent);
	void	EventToolShow					( HISender pSender, HKEvent pEvent );

private:
	FBLayoutRegion	mImportRegion;  // region for import interface
	FBList		mMotionList;  // list the motion files which can be imported
    FBButton    mImportMotionBtn; // import motion

	FBLayoutRegion	mWarpRegion; // region for interface to apply a warp
	FBLabel			mUniformWarpTypeLabel; // label for uniform warp perameters
	FBList			mWarpList;		//list the different test warps that can be apply to motion
	FBLabel			mUniformWarpPercentLabel;
	FBEditNumber	mUniformWarpPercentVal; // magnitude of warp
	FBButton		mWarpBtn;		// apply warp

	FBLayoutRegion mRandomWarpRegion; // region for iunterface for random warp
	FBButton mGenerateRandWarpBtn; // buttonf for generating a random warp
	FBButton mSaveRandWarpBtn;  // button to save the generated random warp values
	FBButton mOpenRandWarpBtn;  // open generated warp values
	FBSpread mRandomWarpDisp; // spreasheet display of generate values
	FBButton mApplyRandWarpBtn; // apply the random warp values

	FBLayoutRegion mDTWRegion; // region for interfaces to perform DTW
	FBLabel			mDTWWarpAmountLabel;  // label for soecifyign the wapr amount
	FBEditNumber	mDTWWarpPercentVal;  // field for amount of warp to apply at each step of predictive warp
	FBLabel			mDTWWarpFreqLabel;	// label to ajgust the fequency at which the motion is  smaple on the predictive warp.
	FBEditNumber	mDTWWarpFreqVal; // fequency at which the motion is  smaple on the predictive warp.
	FBButton		mDTWLogBtn; // button for OPW log toggle, outputs log of warp decision when turned on
	FBButton		mRunPredictiveDTWBtn; // excute predictive DTW
	FBButton		mRunStandardDTWBtn; // execute standard DTW

	FBLayoutRegion	mExportCurvesRegion;  // region for exporting time warp curves
	FBButton		mExportCurvesBtn; // button for exporting time warp curves
	
    FBSystem    mSystem;  // reference to system (i.e. the motionbuilder project/file)
	FBApplication  mApp;
	FBPropertyListCharacter mCharacterList;  // list of characters in motion builder

	pugi::xml_document mXMLDoc; // xml document containing infor on motion which can be imported
	const char* mMotionListXMLFile;

	FBCharacter* mOrginalCharacter;  //characetr with orginal motion applied
	FBCharacter* mWarpedCharacter; //character with warped motion applied
	FBCharacter* mReferenceCharacter; //reference for orginal motion
	FBCharacter* mStandardDTWCharacter;  // character to apply standard DTW algorithm to

	FBTimeSpan mTimespan;   //timespan of current take to informwarp

	FBTime mPrevTime; // the time at which the previsou prediction was made
	double mWarpPercent;
	double mSamplePeriodMillisecond;  // smaple period use for standard DTW which is fixed
	FBTime mSamplePeriodTime;  // time of stadard DTW smaple period
	double mPredSamplePeriodMillisecond;  // predicitive sample period based on the setting in the DTW manager
	FBTime mPredSamplePeriodTime;  // tome object for predicitve sample period.

    // general effector properties
	int mNumEffectors;
    int* mEffectorEnumList;

	// random warp properties
	int mNumRandomWarpEffectors;
	int* mRandomWarpEffectorList;
	RandomWarp* mRandomWarpList;
	int mMaxRandValue;

    // predictive dynamic warping properties
	FBModel** mWarpedEffectorModels;
	FBAnimationNode** mWarpedEffectorTransAnimNodes;
	bool mLogPredictiveDTW;  // determnes data being logged during predictive warp function
	string mLogOutput; //string for storing the lag data

	FBModelCube** mWarpedPredictionMarkers;
	FBAnimationNode** mWarpedPredictionAnimNodes;
	FBAnimationNode* mTimeWarp;
	kLongLong mTimewarpStartLength;   // the length of the timewarp before any charnge are mande to strech or contract the curve

	// orginal effctor properties
	FBModel** mOriginalEffectorModels;
	FBAnimationNode** mOriginalEffectorTransAnimNodes;
	FBAnimationNode** mOriginalEffectorRotationAnimNodes;

	//timewarps
	FBFCurve* mPredictiveWarpCurve;
	FBFCurve* mManualWarpCurve;
	FBFCurve* mDTWWarpCurve;

};

#endif /* __ORTOOL_DTWMANAGER_TOOL_H__ */
