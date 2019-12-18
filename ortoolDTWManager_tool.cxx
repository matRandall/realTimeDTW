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

/**	Tool for Testing Predictive Dynamic Time Wapring
*	Auhtor: Mathew Randall
	Date: 1/2/17
*/

#include "ortoolDTWManager_tool.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <stdlib.h>  // rand
#include <time.h> // time for seed
#include <chrono> // for timeing the spped of the application
#include "pugixml.hpp"

typedef std::chrono::high_resolution_clock Clock;

using namespace std;

//--- Registration defines
#define ORTOOLDTWMANAGER__CLASS	ORTOOLDTWMANAGER__CLASSNAME
#define ORTOOLDTWMANAGER__LABEL	"DTW Manager"
#define ORTOOLDTWMANAGER__DESC	"OR - DTW Manager Tool"

//--- FiLMBOX implementation and registration
FBToolImplementation(	ORTOOLDTWMANAGER__CLASS);
FBRegisterTool(			ORTOOLDTWMANAGER__CLASS,
						ORTOOLDTWMANAGER__LABEL,
						ORTOOLDTWMANAGER__DESC,
						FB_DEFAULT_SDK_ICON		);	// Icon filename (default=Open Reality icon)

/************************************************
 *	FiLMBOX Constructor.
 ************************************************/
bool ORToolDTWManager::FBCreate()
{
	//class parameters
	mMotionListXMLFile = "V:\\Datasets\\CMU_Motion_Capture_Library\\CMU_Library\\ListOfInteractions.xml";

	// set sample period use for standard DTW 
	mSamplePeriodMillisecond = (1000 / FBPlayerControl::TheOne().GetTransportFpsValue());
	mSamplePeriodTime.SetMilliSeconds(mSamplePeriodMillisecond);

	// set arrays for storing time warp into for each effector
	mNumEffectors = 16;

	// set uplist of effectors to map
	mEffectorEnumList = new int[mNumEffectors]{	FBEffectorId::kFBHipsEffectorId,
		FBEffectorId::kFBLeftAnkleEffectorId,
		FBEffectorId::kFBRightAnkleEffectorId,
		FBEffectorId::kFBLeftWristEffectorId,
		FBEffectorId::kFBRightWristEffectorId,
		FBEffectorId::kFBLeftKneeEffectorId,
		FBEffectorId::kFBRightKneeEffectorId,
		FBEffectorId::kFBLeftElbowEffectorId,
		FBEffectorId::kFBRightElbowEffectorId,
		FBEffectorId::kFBChestOriginEffectorId,
		FBEffectorId::kFBChestEndEffectorId,
		FBEffectorId::kFBLeftShoulderEffectorId,
		FBEffectorId::kFBRightShoulderEffectorId,
		FBEffectorId::kFBHeadEffectorId,
		FBEffectorId::kFBLeftHipEffectorId,
		FBEffectorId::kFBRightHipEffectorId};

	// Setup list of effectors to be manipulated by random warp
	mNumRandomWarpEffectors = 5;   // number of effectors
	// list of effectors to randomly warp - need to also update effectorLookup function
	mRandomWarpEffectorList = new int[mNumRandomWarpEffectors]{	FBEffectorId::kFBLeftAnkleEffectorId,
																FBEffectorId::kFBRightAnkleEffectorId,
																FBEffectorId::kFBLeftWristEffectorId,
																FBEffectorId::kFBRightWristEffectorId,
																FBEffectorId::kFBHeadEffectorId};
	// setup emptyList of RandomWarps
	mRandomWarpList = new RandomWarp[mNumRandomWarpEffectors]{};

	// maximumRandom value
	mMaxRandValue = 20;

	// window size
	StartSize[0] = 530;
	StartSize[1] = 600;

	// Tool ui management
	UICreate	();
	UIConfigure	();
	UIReset		();

	// Add tool callbacks
	OnShow.Add(this, (FBCallback)&ORToolDTWManager::EventToolShow);

	return true;
}


/************************************************
 *	FiLMBOX Destruction function.
 ************************************************/
void ORToolDTWManager::FBDestroy()
{
	// Remove tool callbacks
	OnShow.Remove(this, (FBCallback)&ORToolDTWManager::EventToolShow);

}

/************************************************
*	Import motion button
************************************************/
void ORToolDTWManager::EventImportMotionClick(HISender pSender, HKEvent pEvent)
{

	// parts to be mapped
	vector <string> lNodesToMap = { "Hips",
		"LeftUpLeg",
		"LeftLeg",
		"LeftFoot",
		"RightUpLeg",
		"RightLeg",
		"RightFoot",
		"Spine",
		"LeftArm",
		"LeftForeArm",
		"LeftHand",
		"RightArm",
		"RightForeArm",
		"RightHand",
		"Head"
	};

	// create string fo xpath query
	string lXpath = string("Motion[") + to_string(mMotionList.ItemIndex.AsInt() + 1) + "]";

	// create expath query
	pugi::xpath_query lQueryImportMotion(lXpath.c_str());

	// run expath query to create XMLNode
	pugi::xpath_node lImportDataNode = lQueryImportMotion.evaluate_node(mXMLDoc.child("Motion_Library"));

	// construct filename string fro XML data
	string lFilename = string("V:/Datasets/CMU_Motion_Capture_Library/CMU_Library/") 
						+ string(lImportDataNode.node().child_value("Subject_A_Dir")) 
						+ string("/") 
						+ string(lImportDataNode.node().child_value("Subject_A_File"));



	// setup predcitve DTW character *************************

	// import character
	mOrginalCharacter = ImportMotion(lFilename.c_str(), lNodesToMap, "PredictiveDTW");

	// set colour
	FBColor* lColourVal = new FBColor(0.0, 1.0, 0.0);
	FBModel* lModel = mOrginalCharacter->GetModel(FBBodyNodeId::kFBHipsNodeId);
	SetPropForAllNodes(lModel, "Color", lColourVal);
	lModel = mOrginalCharacter->GetCtrlRigModel(FBBodyNodeId::kFBHipsNodeId);
	SetPropForAllNodes(lModel, "Color", lColourVal);

	// move characetr 5 unit in x
	lModel = mOrginalCharacter->GetCtrlRigModel(FBBodyNodeId::kFBReferenceNodeId);
	lModel->Translation = FBVector3d(-100.0, 0.0, 0.0);

	// label character
	labelCharacter(mOrginalCharacter, "Predictive DTW");

	
	
	// setup standard DTW character *************************

	// import character
	mStandardDTWCharacter = ImportMotion(lFilename.c_str(), lNodesToMap, "StandardDTW");

	// set colour
	lColourVal = new FBColor(0.0, 1.0, 1.0);
	lModel = mStandardDTWCharacter->GetModel(FBBodyNodeId::kFBHipsNodeId);
	SetPropForAllNodes(lModel, "Color", lColourVal);
	lModel = mStandardDTWCharacter->GetCtrlRigModel(FBBodyNodeId::kFBHipsNodeId);
	SetPropForAllNodes(lModel, "Color", lColourVal);

	// move characetr 5 unit in x
	lModel = mStandardDTWCharacter->GetCtrlRigModel(FBBodyNodeId::kFBReferenceNodeId);
	lModel->Translation = FBVector3d(-200.0, 0.0, 0.0);

	// label character
	labelCharacter(mStandardDTWCharacter, "Standard DTW");



	// setup warp Character **************************

	// import character
	mWarpedCharacter = ImportMotion(lFilename.c_str(), lNodesToMap, "Warped");

	// set colour
	lColourVal = new FBColor(1.0, 0.0, 0.0);
	lModel = mWarpedCharacter->GetModel(FBBodyNodeId::kFBHipsNodeId);
	SetPropForAllNodes(lModel, "Color", lColourVal);
	lModel = mWarpedCharacter->GetCtrlRigModel(FBBodyNodeId::kFBHipsNodeId);
	SetPropForAllNodes(lModel, "Color", lColourVal);

	// label character
	labelCharacter(mWarpedCharacter, "Warped Motion");


	// setup refernce chracter **************************

	// import character
	mReferenceCharacter = ImportMotion(lFilename.c_str(), lNodesToMap, "Reference");

	// move characetr 5 unit in x
	lModel = mReferenceCharacter->GetCtrlRigModel(FBBodyNodeId::kFBReferenceNodeId);
	lModel->Translation = FBVector3d(100.0, 0.0, 0.0);

	// label character
	labelCharacter(mReferenceCharacter, "Unaltered Reference");

	//finally setup the enimation nodes for the original and warp motions
	SetUpAnimatonNodes();
 
}


/************************************************
 *	Perform warp button
 ************************************************/
void ORToolDTWManager::EventButtonPerformWarp(HISender pSender, HKEvent pEvent)
{
	// check that none is not selected in universal warp options
	if (mWarpList.ItemIndex.AsInt() != 0) {

		//get the timespan of the curenttake
		mSystem.CurrentTake->LocalTimeSpan.GetData(&mTimespan, sizeof(FBTimeSpan));

		//set-up timewarp
		FBTimeWarpManager::TheOne().TimeWarpInitTake(mSystem.CurrentTake);  //set take as container for timewarp
		FBAnimationNode* lTimeWarp = FBTimeWarpManager::TheOne().TimeWarpCreateNew("Uniform Warp");  // create timewarp
		FBTimeWarpManager::TheOne().TimeWarpAddToTake(mSystem.CurrentTake, lTimeWarp);  // add the timewarp to the take

		// get the timewarp curve
		mManualWarpCurve = (FBFCurve*)lTimeWarp->FCurve;

		// modify timewarp curve as required.
		switch (mWarpList.ItemIndex.AsInt()){
		case 1:
			StretchWarp(mManualWarpCurve, mUniformWarpPercentVal.Value);
			break;
		case 2:
			ShrinkWarp(mManualWarpCurve, mUniformWarpPercentVal.Value);
			break;
		case 3:
			FastSlowWarp(mManualWarpCurve, mUniformWarpPercentVal.Value);
			break;
		case 4:
			SlowFastWarp(mManualWarpCurve, mUniformWarpPercentVal.Value);
			break;
		case 5:
			SlowInSlowOutWarp(mManualWarpCurve, mUniformWarpPercentVal.Value);
			break;
		case 6:
			FastInFastOutWarp(mManualWarpCurve, mUniformWarpPercentVal.Value);
		}

		//check if take needs expanding to take into account new warp
		FBTime lStop = mTimespan.GetStop();
		FBTime lWarpEnd = mManualWarpCurve->Keys[mManualWarpCurve->Keys.GetCount() - 1].Time;
		if (lWarpEnd > lStop){
			mTimespan.Set(mTimespan.GetStart(), lWarpEnd);
			mSystem.CurrentTake->LocalTimeSpan.SetData(&mTimespan);
		}

		//apply the warp to all animated elements in the control rig
		FBModel* lModel = mWarpedCharacter->GetCtrlRigModel(FBBodyNodeId::kFBReferenceNodeId);
		addAllNodesToWarp(lModel, lTimeWarp);

		//set the first key of time warp to cubic spline interpolation
		SetFirstKeyToCubic(mManualWarpCurve);

		// delete the timewarp curve
		FBTimeWarpManager::TheOne().RemoveTimeWarpFromScene(lTimeWarp);

	}

}

/************************************************
*	Generate Random warp button
************************************************/
void ORToolDTWManager::EventButtonGenerateRandomWarp(HISender pSender, HKEvent pEvent)
{
	
	// empty randomwarp list
	mRandomWarpList[mNumRandomWarpEffectors] = {};

	// initalise random seed
	srand(time(NULL));

	// generate the random values
	for (int i = 0; i < mNumRandomWarpEffectors; i++){
		RandomWarp lRandomWarp;
		lRandomWarp.effector = mRandomWarpEffectorList[i];
		lRandomWarp.warpType = rand() % mWarpList.Items.GetCount();  //rand generates very large whole number hence the use of modulus
		lRandomWarp.warpValue = rand() % mMaxRandValue;

		mRandomWarpList[i] = lRandomWarp;

	}

	PopulateRandomWarpDisp();

}

/************************************************
*	Save Random warp button
************************************************/
void ORToolDTWManager::EventButtonSaveRandomWarp(HISender pSender, HKEvent pEvent)
{
	
	//setup and execute file pop window
	FBFilePopup lFilePopUp; // create file popup opject
	lFilePopUp.Style = FBFilePopupStyle::kFBFilePopupSave; // set to save file
	lFilePopUp.Caption = "Save Generated Values";
	lFilePopUp.Filter = "*.xml"; //set-up xml extension
	lFilePopUp.Execute(); // exucute

	// setup XML doc
	pugi::xml_document lXMLDoc;  // creeat document
	pugi::xml_node lRootNode = lXMLDoc.append_child("Random_Warp_Values");  //set root
	
	pugi::xml_node lWarpNode;  // create a empty warp node to reuse in loop

	// loop through each effector outputting warp data
	for (int i = 0; i < mNumRandomWarpEffectors; i++){
		lWarpNode = lRootNode.append_child("Warp");  // create warp node

		// create node for wach warp data
		lWarpNode.append_child("Effector_ID").text() = mRandomWarpList[i].effector;
		lWarpNode.append_child("Effector_Name").text() = EffectorLookup(mRandomWarpList[i].effector);
		lWarpNode.append_child("Warp_Type_ID").text() = mRandomWarpList[i].warpType;
		lWarpNode.append_child("Warp_Type_Name").text() = mWarpList.Items[mRandomWarpList[i].warpType];
		lWarpNode.append_child("Warp_Value").text() = mRandomWarpList[i].warpValue;
	}

	lXMLDoc.save_file(lFilePopUp.FullFilename);  // save xml file

}

/************************************************
*	Open Random warp button
************************************************/
void ORToolDTWManager::EventButtonOpenRandomWarp(HISender pSender, HKEvent pEvent)
{
	//setup and execute file pop window
	FBFilePopup lFilePopUp; // create file popup opject
	lFilePopUp.Style = FBFilePopupStyle::kFBFilePopupOpen; // set to Open file
	lFilePopUp.Caption = "Open Generated Values";
	lFilePopUp.Filter = "*.xml"; //set-up xml extension
	lFilePopUp.Execute(); // exucute

	// empty the warp values
	mRandomWarpList[mNumRandomWarpEffectors] = {};
	int lCounter = 0;

	pugi::xml_document lXMLDoc;
	pugi::xml_parse_result lXMLRes = lXMLDoc.load_file(lFilePopUp.FullFilename);

	for (pugi::xml_node lWarpNode = lXMLDoc.child("Random_Warp_Values").child("Warp"); lWarpNode; lWarpNode = lWarpNode.next_sibling("Warp"))
	{
		mRandomWarpList[lCounter].effector = lWarpNode.child("Effector_ID").text().as_int();
		mRandomWarpList[lCounter].warpType = lWarpNode.child("Warp_Type_ID").text().as_int();
		mRandomWarpList[lCounter].warpValue = lWarpNode.child("Warp_Value").text().as_int();
		
		lCounter++;

	}

	PopulateRandomWarpDisp();

}


/************************************************
*	Apply Random warp button
************************************************/
void ORToolDTWManager::EventButtonApplyRandomWarp(HISender pSender, HKEvent pEvent)
{
	string lWarpName = "";
	
	for (int i = 0; i < mNumRandomWarpEffectors; i++){
		// check that none is not selected in universal warp options
		if (mRandomWarpList[i].warpType != 0) {

			//get the timespan of the curenttake
			mSystem.CurrentTake->LocalTimeSpan.GetData(&mTimespan, sizeof(FBTimeSpan));

			//set-up timewarp
			FBTimeWarpManager::TheOne().TimeWarpInitTake(mSystem.CurrentTake);  //set take as container for timewarp
			FBAnimationNode* lTimeWarp = FBTimeWarpManager::TheOne().TimeWarpCreateNew(EffectorLookup(mRandomWarpList[i].effector));  // create timewarp
			FBTimeWarpManager::TheOne().TimeWarpAddToTake(mSystem.CurrentTake, lTimeWarp);  // add the timewarp to the take

			// create new timewarp curve
			FBFCurve* lFCurve = (FBFCurve*)lTimeWarp->FCurve;

			// modify timewarp curve as required.
			switch (mRandomWarpList[i].warpType){
			case 1:
				StretchWarp(lFCurve, mRandomWarpList[i].warpValue);
				break;
			case 2:
				ShrinkWarp(lFCurve, mRandomWarpList[i].warpValue);
				break;
			case 3:
				FastSlowWarp(lFCurve, mRandomWarpList[i].warpValue);
				break;
			case 4:
				SlowFastWarp(lFCurve, mRandomWarpList[i].warpValue);
				break;
			case 5:
				SlowInSlowOutWarp(lFCurve, mRandomWarpList[i].warpValue);
				break;
			case 6:
				FastInFastOutWarp(lFCurve, mRandomWarpList[i].warpValue);
			}

			//check if take needs expanding to take into account new warp
			FBTime lStop = mTimespan.GetStop();
			FBTime lWarpEnd = lFCurve->Keys[lFCurve->Keys.GetCount() - 1].Time;
			if (lWarpEnd > lStop){
				mTimespan.Set(mTimespan.GetStart(), lWarpEnd);
				mSystem.CurrentTake->LocalTimeSpan.SetData(&mTimespan);
			}

			//get effector to apply warp to
			FBModel* lModel = mWarpedCharacter->GetEffectorModel(FBEffectorId(mRandomWarpList[i].effector));

			// set the IK T Blend value to 100 for the effector
			double lIKBlendVal = 100;
			FBProperty* lProp = lModel->PropertyList.Find("IK Reach Translation");
			lProp->SetData(&lIKBlendVal);
			lProp = lModel->PropertyList.Find("IK Reach Rotation");
			lProp->SetData(&lIKBlendVal);

			// apply warp to rotation
			FBAnimationNode* lAnimNode = lModel->Rotation.GetAnimationNode();
			FBTimeWarpManager::TheOne().ApplyTimeWarp(mSystem.CurrentTake, &lModel->Rotation, lTimeWarp);
			FBTimeWarpManager::TheOne().TimeWarpMergeCurveNode(mSystem.CurrentTake, &lModel->Rotation, lAnimNode, lTimeWarp);

			// applywarp to transaltion
			lAnimNode = lModel->Translation.GetAnimationNode();
			FBTimeWarpManager::TheOne().ApplyTimeWarp(mSystem.CurrentTake, &lModel->Rotation, lTimeWarp);
			FBTimeWarpManager::TheOne().TimeWarpMergeCurveNode(mSystem.CurrentTake, &lModel->Rotation, lAnimNode, lTimeWarp);

		}
	}
}

/************************************************
*	Run Standard DTW Button
*	This finction maps smaples in roginal character motion to samples in warped character motion
*	Once mapping is done, the mapping information is used to create warps
*	The warps are all placed before they are moved to avoid conflicts of warping warped curves.
************************************************/
void ORToolDTWManager::EventButtonRunStandardDTW(HISender pSender, HKEvent pEvent){

	// get the time span of the orginal and warp character animations based on finding the last key in animation curves.
	FBTime lWarpedAnimLength = FindLastKey(mWarpedCharacter);
	FBTime lOriginalAnimLength = FindLastKey(mStandardDTWCharacter);
	
	// create of set of empty vectors in which effector positions can be loaded for the orginal motion
	FBVector3d* lOriginalSamplePosArr = new FBVector3d[mNumEffectors];

	// set variables for determining frames with the shortest distance
	double lSmallestDist; // smallest distand between frames
	double lTotalDist; // total for all effectors
	double lDist; // stores distance of current effectors being tested
	int lOriginalSampleNo = 0;
	int lWarpedSampleNo = 0;
	int lBestMatchSampleNo;
	FBProgress lProgress;
	string lProgText;
	int lProgPerc;

	// create time variabkle do determine the time on of the orginal and warp motion that is being tested
	FBTime lOriginalSampleTime;
	FBTime lWarpSampleTime;

	// set the orginal sample time to 0
	lOriginalSampleTime.SetMilliSeconds(0);

	// determine number of smaple and setup array of samples
	int lNoSamples = int(lOriginalAnimLength.GetMilliSeconds() / mSamplePeriodTime.GetMilliSeconds()) + 1;
	int* lWarpMapArr = new int[lNoSamples];

	//setup progress bar
	lProgress.Caption = "Progress";
	lProgress.Text = "Progress Text";

	// loop through original motion samples
	do{

		// set biggest distance back to silly big number
		lSmallestDist = 1000;

		// reset best match sample
		lBestMatchSampleNo = 0;
		lWarpedSampleNo = 0;

		// get the positions of the orginal character's end effectors at orginal sample time
		for (int i = 0; i < mNumEffectors; i++){
			lOriginalSamplePosArr[i] = EvaluateNodes(mOriginalEffectorTransAnimNodes[i], lOriginalSampleTime);
		}

		// set warp sample time back to zero
		lWarpSampleTime.SetMilliSeconds(0);

		// loop through warped motion samples
		do{

			// reset the total distance
			lTotalDist = 0;

			//get distance between each corresponding effectors in orginal and warped character
			for (int i = 0; i < mNumEffectors; i++){

				// get the position of the warped character's end effector at the
				FBVector3d lWarpedSamplePos = EvaluateNodes(mWarpedEffectorTransAnimNodes[i], lWarpSampleTime);

				// get the distance between the orginal and warp character end effectors
				lDist = calcDistance3d(lOriginalSamplePosArr[i], lWarpedSamplePos);

				// add to the running distance total
				lTotalDist += lDist;

			}

			// check if this is best matching cobination of frames
			if (lTotalDist < lSmallestDist){
				lSmallestDist = lTotalDist;
				lBestMatchSampleNo = lWarpedSampleNo;
			}

			// add sample period to sample time of warp motion, ready for next loop
			lWarpSampleTime += mSamplePeriodTime;

			// incrementSampleNo
			lWarpedSampleNo++;

		} while (lWarpSampleTime < lWarpedAnimLength);  // check if end of take is reached

		// check if mapping first sample
		if (lOriginalSampleNo == 0){
			// if first sample then save best match
			lWarpMapArr[0] = lBestMatchSampleNo;
		}
		// if not first sample then check warp is continous and not going backwards		
		else if(lBestMatchSampleNo < lWarpMapArr[lOriginalSampleNo - 1]){
			// if going backwards map smaple to same posiiton as previous sample
			lWarpMapArr[lOriginalSampleNo] = lWarpMapArr[lOriginalSampleNo - 1];
		}
		// if not first sample then check slope limit not greater than
		else if (lBestMatchSampleNo >= lWarpMapArr[lOriginalSampleNo - 1] + 2){
			// if slop to great then limit it
			lWarpMapArr[lOriginalSampleNo] = lWarpMapArr[lOriginalSampleNo - 1] + 2;
		}
		else {
			// if no problems with continouty and slope the map smaple to best match
			lWarpMapArr[lOriginalSampleNo] = lBestMatchSampleNo;
		}

		// add sample period to smaple time of original motion, ready for next loop
		lOriginalSampleTime += mSamplePeriodTime;

		// increment sampleNo
		lOriginalSampleNo++;

		// update the progress bar
		//lProgPerc = (lOriginalSampleTime.GetMilliSeconds() / lOriginalAnimLength.GetMilliSeconds()) * 1000;
		//lProgText = to_string(lProgPerc) + "%";
		//lProgress.Text = lProgText.c_str();

	} while (lOriginalSampleTime < lOriginalAnimLength); // check if end of take is raeched


	//************ create timewarp **************

	//set-up timewarp
	FBTimeWarpManager::TheOne().TimeWarpInitTake(mSystem.CurrentTake);  //set take as container for timewarp
	FBAnimationNode* lTimeWarp = FBTimeWarpManager::TheOne().TimeWarpCreateNew("Standard DTW");  // create timewarp
	FBTimeWarpManager::TheOne().TimeWarpAddToTake(mSystem.CurrentTake, lTimeWarp);  // add the timewarp to the take

	// get the timewarp curve
	mDTWWarpCurve = (FBFCurve*)lTimeWarp->FCurve;

	// create vector to store samples to be warped
	vector<int> lSparseSamples;

	//check through sample to identify position where warps need to be inserted
	for (int i = 1; i < lNoSamples; i++){
		// check if warp is not continous
		if (lWarpMapArr[i] != lWarpMapArr[i - 1] + 1){
			
			// check that this sample is not mapped to the same time as the place as the next sample.
			if (!(lWarpMapArr[i + 1] == lWarpMapArr[i])){
				lSparseSamples.push_back(i);
			}
		}
	}
	// check if the last smaple has been added to sparse set
	if (lSparseSamples.back() != (lNoSamples - 1)){
		lSparseSamples.push_back(lNoSamples - 1);
	}

	// evaluate value of spares sample
	// then add key for each sparse sample wit new time an existing value

	double* lKeyValues = new double[lSparseSamples.size()];
	for (int i = 0; i < lSparseSamples.size(); ++i){
		FBTime lKeyTime = mSamplePeriodTime * lSparseSamples[i];
		lKeyValues[i] = mDTWWarpCurve->Evaluate(lKeyTime);
	}

	// delete the last key -  so this doesn't messup the warp
	mDTWWarpCurve->KeyDelete(mDTWWarpCurve->Keys.GetCount() - 1, mDTWWarpCurve->Keys.GetCount() - 1);

	// move each key to warped postions
	for (int i = 0; i < lSparseSamples.size(); ++i){
		mDTWWarpCurve->KeyAdd(mSamplePeriodTime * lWarpMapArr[lSparseSamples[i]], lKeyValues[i]);
	}
	
	//set the first key of time warp to cubic spline interpolation
	SetFirstKeyToCubic(mDTWWarpCurve);

	//apply the warp to all animated elements in the control rig
	FBModel* lModel = mStandardDTWCharacter->GetCtrlRigModel(FBBodyNodeId::kFBReferenceNodeId);
	addAllNodesToWarp(lModel, lTimeWarp);

	// delete the timewarp curve
	FBTimeWarpManager::TheOne().RemoveTimeWarpFromScene(lTimeWarp);
}



/************************************************
*	Run Predictive DTW Button
************************************************/
void ORToolDTWManager::EventButtonRunPredictiveDTW(HISender pSender, HKEvent pEvent){

	// initalise DTW properties
	mPrevTime.SetMilliSeconds(0);// reset the time of the previous prediction step to 0
	mWarpPercent = mDTWWarpPercentVal.Value;

	// set sample period use for predictive DTW
	mPredSamplePeriodMillisecond = (1000. / mDTWWarpFreqVal.Value);
	mPredSamplePeriodTime.SetMilliSeconds(mPredSamplePeriodMillisecond);

	//set-up list of reference cubes
	mWarpedPredictionMarkers = new FBModelCube*[mNumEffectors];
	mWarpedPredictionAnimNodes = new FBAnimationNode*[mNumEffectors];

	// for each effector
	for (int i = 0; i < mNumEffectors; i++){

		// create reference cubes
		string lMarkerName = "Marker" + string(mWarpedEffectorModels[i]->Name);
		mWarpedPredictionMarkers[i] = new FBModelCube(lMarkerName.c_str());
		mWarpedPredictionMarkers[i]->Show = true;
		mWarpedPredictionMarkers[i]->Translation.SetAnimated(true);
		mWarpedPredictionAnimNodes[i] = mWarpedPredictionMarkers[i]->Translation.GetAnimationNode();

	}

	//set-up timewarp
	FBTimeWarpManager::TheOne().TimeWarpInitTake(mSystem.CurrentTake);  //set take as container for timewarp
	mTimeWarp = FBTimeWarpManager::TheOne().TimeWarpCreateNew("DTWWarpmap");  // create timewarp
	FBTimeWarpManager::TheOne().TimeWarpAddToTake(mSystem.CurrentTake, mTimeWarp);  // add the timewarp to the take

	//add effectors to warp
	for (int i = 0; i < mNumEffectors; i++){
		FBTimeWarpManager::TheOne().ApplyTimeWarp(mSystem.CurrentTake, &mOriginalEffectorModels[i]->Rotation, mTimeWarp);
		FBTimeWarpManager::TheOne().ApplyTimeWarp(mSystem.CurrentTake, &mOriginalEffectorModels[i]->Translation, mTimeWarp);
	}

	// store reefernce to curve
	mPredictiveWarpCurve = (FBFCurve*)mTimeWarp->FCurve;
	mTimewarpStartLength = FBTime(mPredictiveWarpCurve->Keys[mPredictiveWarpCurve->Keys.GetCount() - 1].Time).GetMilliSeconds();

	// check if log button is pressed and setup log file
	
	// set logging to false
	mLogPredictiveDTW = false;

	//check if warp curves are poresent
	if (!mManualWarpCurve || !mDTWWarpCurve){

		// create errore message showing which warp curves are not present
		string lErrorStr = "The following time warps are messing: \n \n";

		if (!mManualWarpCurve){
			lErrorStr.append("- Manual Uniform Time Warp\n");
		}

		if (!mDTWWarpCurve){
			lErrorStr.append("- Standard Time Warp\n");
		}

		// display message box
		FBMessageBox("No Timewarps Box", lErrorStr.c_str(), "Continue", NULL, NULL, 1);

	}
	// check is log button has been set
	else if (mDTWLogBtn.State == true){

		// set the predictive warp to true
		mLogPredictiveDTW = true;

		// initialise output string
		mLogOutput = "\"Time\",\"Manual_Curve\",\"DTW_Curve\",\"Predictive_Curve\",\"Warp_Direction\"\n";

	}

	// set-up call back on evlkuation pipeline
	FBEvaluateManager::TheOne().OnEvaluationPipelineEvent.Add(this, (FBCallback)&ORToolDTWManager::EventHandler);
	
	// start playback from beginning for one loop
	FBPlayerControl::TheOne().Goto(FBTime(FBPlayerControl::TheOne().LoopStart) + mSamplePeriodTime);  // start on frame 2
	FBPlayerControl::TheOne().LoopActive = false;
	FBPlayerControl::TheOne().Play();

}

/************************************************
*	Evaluation callback function
*	supporting the prective DTW function
*   This is where the progress of the warp is updated every frame
************************************************/
void ORToolDTWManager::EventHandler(HISender pSender, HKEvent pEvent){
	
	// check if animation is playing back
	if (FBPlayerControl::TheOne().IsPlaying == true){

		// get current time and frame
		FBTime lCurrentTime = mSystem.LocalTime;
		
		// check if the current frame is a new frame
		if (lCurrentTime > (mPrevTime + mPredSamplePeriodTime)){

			// get start time for timer
			auto t1 = Clock::now();

			kLongLong lCurrentTimeMS = lCurrentTime.GetMilliSeconds();

			// get the index and time of the last key in the time warp
			int lLastWarpKey = mPredictiveWarpCurve->Keys.GetCount() - 1;
			FBTime lEndWarpTime = mPredictiveWarpCurve->Keys[lLastWarpKey].Time;

			//determine the current total amount of warp at a ratio of warped time over orginal time
			kLongLong lCurrentTimewarpLeghtMS = lEndWarpTime.GetMilliSeconds();
			double lCurrentWarpRatio = double(mTimewarpStartLength) / double(lCurrentTimewarpLeghtMS);

			// get previous and future time sample posiitons
			FBTime lPreviousSampleTime = lCurrentTime - mPredSamplePeriodTime;
			FBTime lNextSampleTime = lCurrentTime + mPredSamplePeriodTime;
			FBTime lTwoSamplesAhead = lCurrentTime + mPredSamplePeriodTime + mPredSamplePeriodTime;

			// detremine time position multiplied by warp ratio
			FBTime lCurrentTimeWarped = lCurrentTime * lCurrentWarpRatio;
			FBTime llNextSampleTimeWarped = lNextSampleTime * lCurrentWarpRatio;
			FBTime lTwoSamplesAheadWarped = lTwoSamplesAhead * lCurrentWarpRatio;

			//cout << "Frame: " << lCurrentTime.GetFrame();
			cout << lCurrentTime.GetFrame() << ":- Current Time:" << lCurrentTime.GetMilliSeconds() << ", Current Warped Time:" << lCurrentTimeWarped.GetMilliSeconds() << endl;

			// setup variable to accumulate the difference between frames based on distance
			double lDiffCurrentSample = 0;
			double lDiffNextSample = 0;
			double lDiffTwoSampleAhead = 0;

			// do actions for each effctor	
			for (int i = 0; i < mNumEffectors; i++){

				// get the position of the warped efectors for the current and previous frame
				FBVector3d lWarpedPosCurrentFrame = EvaluateNodes(mWarpedEffectorTransAnimNodes[i], lCurrentTime);
				FBVector3d lWarpedPosPrevFrame = EvaluateNodes(mWarpedEffectorTransAnimNodes[i], lPreviousSampleTime);

				// predict the motion of the warped motion
				FBVector3d lWarpedPredictedPos = lPredictPos(lWarpedPosPrevFrame, lWarpedPosCurrentFrame);

				// position markers to represent position of predicted motionn
				add3DVectorKey(mWarpedPredictionAnimNodes[i], lWarpedPredictedPos, lNextSampleTime);

				// get the position of original motion current, +1 and + 2 time periods ahead
				FBVector3d lOriginalPosCurrentFrame = EvaluateNodes(mOriginalEffectorTransAnimNodes[i], lCurrentTimeWarped);
				FBVector3d lOriginalPosNextFrame = EvaluateNodes(mOriginalEffectorTransAnimNodes[i], llNextSampleTimeWarped);
				FBVector3d lOriginalPos2FramesAhead = EvaluateNodes(mOriginalEffectorTransAnimNodes[i], lTwoSamplesAheadWarped);
				
				// accumulate the cost function (difference) between prediction and original motion current, +1 and + 2 time periods ahead
				lDiffCurrentSample += calcDistance3d(lWarpedPredictedPos, lOriginalPosCurrentFrame);
				lDiffNextSample += calcDistance3d(lWarpedPredictedPos, lOriginalPosNextFrame);
				lDiffTwoSampleAhead += calcDistance3d(lWarpedPredictedPos, lOriginalPos2FramesAhead);

			}

			// determine if there is aneed to extend or shrink motion by warp factor
			float lWarpFactor = 0;

			if ((lDiffCurrentSample < lDiffNextSample) && (lDiffCurrentSample < lDiffTwoSampleAhead)){
				lWarpFactor = mWarpPercent;
			}
			else if ((lDiffTwoSampleAhead < lDiffCurrentSample) && (lDiffTwoSampleAhead < lDiffNextSample)){
				lWarpFactor = -mWarpPercent;
			}

			

			// do warp if required
			if (lWarpFactor != 0){

				//edit time warp curve		
			
				//pin time warp curve at current frame by adding a warp key
				float lCurrentWarpVal = mPredictiveWarpCurve->Evaluate(lCurrentTime);
				mPredictiveWarpCurve->KeyAdd(lCurrentTime, lCurrentWarpVal);

				//edit time position of last key to speed up or slow down motion
				lLastWarpKey = mPredictiveWarpCurve->Keys.GetCount() - 1;  // need to determine the last key again as a key has been added.
				FBTime lTimeLeft = (lEndWarpTime - lCurrentTime);
				FBTime lNewWarpEndTime = (lTimeLeft * (lWarpFactor / 100)) + lEndWarpTime;
					
				cout << "; End Warp Time: " << lEndWarpTime.GetMilliSeconds() << "; New End Warp Time: " << lNewWarpEndTime.GetMilliSeconds() << endl;
				cout << "Test Value: " << FBTime((lEndWarpTime - lCurrentTime) * (lWarpFactor / 100)).GetMilliSeconds() << endl;
					
				mPredictiveWarpCurve->Keys[lLastWarpKey].Time = lNewWarpEndTime;

				//********* output log

				if (mLogPredictiveDTW) {

					// initiate new row
					string lRow = "";

					// output sample time
					lRow.append(to_string(lCurrentTime.GetSecondDouble()));
					lRow.append(",");

					// output manual curve value
					lRow.append(to_string(mManualWarpCurve->Evaluate(lCurrentTime)));
					lRow.append(",");

					// output DTW curve sample 
					lRow.append(to_string(mDTWWarpCurve->Evaluate(lCurrentTime)));
					lRow.append(",");

					// output predictive DTW curve sample 
					lRow.append(to_string(mPredictiveWarpCurve->Evaluate(lCurrentTime)));
					lRow.append(",");

					// output direction of warp
					if (lWarpFactor < 0) {
						lRow.append("-1");
					}
					else if (lWarpFactor > 1){
						lRow.append("1");
					}
				
					// end row
					lRow.append("\n");

					// add row to output
					mLogOutput.append(lRow);

				}

				//*********** end output log
				
			}

			// set previous frame to current frame
			mPrevTime = lCurrentTime;

			// get end time and print time taken
			auto t2 = Clock::now();
			cout << "Time Taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " Milliseconds" << endl;
		}

	}
	else {
		
		cout << "completing " << endl;

		//set the first key of time warp to cubic spline interpolation
		SetFirstKeyToCubic(mPredictiveWarpCurve);

		//check if the playback is on the last frame.
		if (FBTime(mSystem.LocalTime).GetFrame() == FBTime(FBPlayerControl::TheOne().LoopStop).GetFrame()){
			
			// remove the callback
			FBEvaluateManager::TheOne().OnEvaluationPipelineEvent.Remove(this, (FBCallback)&ORToolDTWManager::EventHandler);

			//************* stop outputting log
			if (mLogPredictiveDTW){

				// pop up request for file name
				FBFilePopup lFilePopUp; // create file popup opject
				lFilePopUp.Style = FBFilePopupStyle::kFBFilePopupSave; // set to save file
				lFilePopUp.Caption = "Save Log of Predictive DTW Process";
				lFilePopUp.Filter = "*.csv"; //set-up csv extension
				lFilePopUp.Execute(); // exucute

				// output the file
				ofstream lFile(lFilePopUp.FullFilename);
				if (lFile.is_open()){
					cout << "file is open" << endl;
					lFile << mLogOutput;
				}
				else {
					cout << "can;t open file" << endl;
				}
				lFile.close();

			}
		}
	}

 
}


/************************************************
*	Export Timewarp Curves button
************************************************/
void ORToolDTWManager::EventButtonExportCurves(HISender pSender, HKEvent pEvent){

	// check of all the warp curves are present
	if (!mPredictiveWarpCurve || !mManualWarpCurve || !mDTWWarpCurve){

		// create errore message showing which warp curves are not present
		string lErrorStr = "The following time warps are messing: \n \n";
		
		if (!mManualWarpCurve){
			lErrorStr.append("- Manual Uniform Time Warp\n");
		}
		if (!mPredictiveWarpCurve){
			lErrorStr.append("- Predictive Time Warp\n");
		}
		if (!mDTWWarpCurve){
			lErrorStr.append("- Standard Time Warp\n");
		}

		// display message box
		FBMessageBox("No Timewarps Box", lErrorStr.c_str(), "Continue", NULL, NULL, 1);

	}
	// if all time waprs are present then continue with export.
	else {
		
		// set local properties
		int lSamplesPerSec = 50;  //sample rate
		string lRow = "";  // row string reset on each loop to produce a string
		FBTime lSampleTime; // time of the current sample
		FBTime lTotalTime = mManualWarpCurve->Keys[mManualWarpCurve->Keys.GetCount() - 1].Time;  //total time
		// calc the number of sample + 1 for end sample
		int lNoSamples = floor(lTotalTime.GetMilliSeconds() / (1000 / lSamplesPerSec)) + 1;  

		// pop up request for file name
		FBFilePopup lFilePopUp; // create file popup opject
		lFilePopUp.Style = FBFilePopupStyle::kFBFilePopupSave; // set to save file
		lFilePopUp.Caption = "Save Generated Values";
		lFilePopUp.Filter = "*.csv"; //set-up csv extension
		lFilePopUp.Execute(); // exucute
		
		// create title string
		string lOutput = "\"Sample_No\",\"Time\",\"Manual_Curve\",\"DTW_Curve\",\"Predictive_Curve\"\n";

		// create row for each sample
		for (int i = 0; i < lNoSamples; i++){
			
			// set the time of the sample
			lSampleTime.SetMilliSeconds((1000. / lSamplesPerSec) * i);

			cout << lSampleTime.GetMilliSeconds() << endl;
		
			// reset row to blank string
			lRow = "";

			// output sample no
			lRow.append(to_string(i));
			lRow.append(",");

			// output sample time
			lRow.append(to_string(lSampleTime.GetSecondDouble()));
			lRow.append(",");

			// output manual curve value
			lRow.append(to_string(mManualWarpCurve->Evaluate(lSampleTime)));
			lRow.append(",");

			// output DTW curve sample 
			lRow.append(to_string(mDTWWarpCurve->Evaluate(lSampleTime)));
			lRow.append(",");

			// output Predictive curve sample
			lRow.append(to_string(mPredictiveWarpCurve->Evaluate(lSampleTime)));

			lOutput.append("\n");

			lOutput.append(lRow);

		}

		// output the file
		ofstream lFile(lFilePopUp.FullFilename);
		if (lFile.is_open()){
			cout << "file is open" << endl;
			lFile << lOutput;
		}
		else {
			cout << "can;t open file" << endl;
		}
		lFile.close();
	}

}


/************************************************
*	On System Idle Function
************************************************/
void ORToolDTWManager::EventIdle(HISender pSender, HKEvent pEvent){
	
	if (FBPlayerControl::TheOne().IsPlaying == true){

		FBTime lCurrentTime = mSystem.LocalTime;
		cout << lCurrentTime.GetMilliSeconds() << endl;

	}

}

/************************************************
 *	Handle tool activation (selection/unselection).
 ************************************************/
void ORToolDTWManager::EventToolShow(HISender pSender, HKEvent pEvent)
{
	FBEventShow lEvent( pEvent );
	if( lEvent.Shown )
	{
		UIReset();
	}
	else
	{
	}
}

/************************************************
 *	UI creation.
 ************************************************/
void ORToolDTWManager::UICreate()
{
	int lS = 10;
	int lH = 18;
	int lW = 500;
	int lListW = 300;
	int lBtnW = 150;

	// Define import region
	AddRegion("ImportRegion", "ImportRegion",
										lS, kFBAttachLeft, "", 1.0,
										lH, kFBAttachTop, "", 1.0,
										lW, kFBAttachNone, "", 1.0,
										50, kFBAttachNone, NULL, 1.0);

	AddRegion("MotionList", "MotionList",
										lS, kFBAttachLeft, "ImportRegion", 1.0,
										lH, kFBAttachTop, "ImportRegion", 1.0,
										lListW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);
	
	AddRegion("ImportMotionBtn", "ImportMotionBtn",
										lS, kFBAttachRight, "MotionList", 1.0,
										lH, kFBAttachTop, "ImportRegion", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	SetControl("ImportRegion", mImportRegion);
	SetControl("MotionList", mMotionList);
	SetControl("ImportMotionBtn", mImportMotionBtn);

	// define uniform warp Warp Region
	AddRegion("UniformWarpRegion", "UnifromWarpRegion",
										lS, kFBAttachLeft, "", 1.0,
										lH, kFBAttachBottom, "ImportRegion", 1.0,
										lW, kFBAttachNone, "", 1.0,
										75, kFBAttachNone, NULL, 1.0);

	AddRegion("UniformWarpTypeLabel", "UniformWarpTypeLabel",
										lS, kFBAttachLeft, "UniformWarpRegion", 1.0,
										lH, kFBAttachTop, "UniformWarpRegion", 1.0,
										45, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("SelectWarpList", "SelectWarpList",
										lS, kFBAttachRight, "UniformWarpTypeLabel", 1.0,
										lH, kFBAttachTop, "UniformWarpRegion", 1.0,
										200, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("UniformWarpPercentLabel", "UniformWarpPercentLabel",
										lH, kFBAttachRight, "SelectWarpList", 1.0,
										lH, kFBAttachTop, "UniformWarpRegion", 1.0,
										80, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("UniformWarpPercentVal", "UniformWarpPercentVal",
										lS, kFBAttachRight, "UniformWarpPercentLabel", 1.0,
										lH, kFBAttachTop, "UniformWarpRegion", 1.0,
										45, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);
	
	AddRegion("ApplyUniformWarpBtn", "ApplyTestWarpBtn",
										lS, kFBAttachLeft, "UniformWarpRegion", 1.0,
										lS, kFBAttachBottom, "SelectWarpList", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	SetControl("UniformWarpRegion", mWarpRegion);
	SetControl("UniformWarpTypeLabel", mUniformWarpTypeLabel);
	SetControl("SelectWarpList", mWarpList);
	SetControl("UniformWarpPercentLabel", mUniformWarpPercentLabel);
	SetControl("UniformWarpPercentVal", mUniformWarpPercentVal);
	SetControl("ApplyUniformWarpBtn", mWarpBtn);

	// define random warp region
	AddRegion("RandomWarpRegion", "RandomWarpRegion",
										lS, kFBAttachLeft, "", 1.0,
										lH, kFBAttachBottom, "UniformWarpRegion", 1.0,
										lW, kFBAttachNone, "", 1.0,
										220, kFBAttachNone, NULL, 1.0);

	AddRegion("GenerateRandomWarpBTN", "GenerateRandomWarpBTN",
										lS, kFBAttachLeft, "RandomWarpRegion", 1.0,
										lH, kFBAttachTop, "RandomWarpRegion", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("SaveRandomWarpBTN", "SaveRandomWarpBTN",
										lS, kFBAttachRight, "GenerateRandomWarpBTN", 1.0,
										lH, kFBAttachTop, "RandomWarpRegion", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("OpenRandomWarpBTN", "OpenRandomWarpBTN",
										lS, kFBAttachRight, "SaveRandomWarpBTN", 1.0,
										lH, kFBAttachTop, "RandomWarpRegion", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("RandomWarpDisp", "RandomWarpDisp",
										lS, kFBAttachLeft, "RandomWarpRegion", 1.0,
										lS, kFBAttachBottom, "GenerateRandomWarpBTN", 1.0,
										-lS, kFBAttachRight, "RandomWarpRegion", 1.0,
										140, kFBAttachNone, "", 1.0);

	AddRegion("ApplyRandomWarpBTN", "GenerateRandomWarpBTN",
										lS, kFBAttachLeft, "RandomWarpRegion", 1.0,
										lS, kFBAttachBottom, "RandomWarpDisp", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	SetControl("RandomWarpRegion", mRandomWarpRegion);
	SetControl("GenerateRandomWarpBTN", mGenerateRandWarpBtn);
	SetControl("SaveRandomWarpBTN", mSaveRandWarpBtn);
	SetControl("OpenRandomWarpBTN", mOpenRandWarpBtn);
	SetControl("RandomWarpDisp", mRandomWarpDisp);
	SetControl("ApplyRandomWarpBTN", mApplyRandWarpBtn);


	//define dynamic timewarp interface
	AddRegion("ExecuteDTWRegion", "ExecuteDTWRegion",
										lS, kFBAttachLeft, "", 1.0,
										lH, kFBAttachBottom, "RandomWarpRegion", 1.0,
										lW, kFBAttachNone, "", 1.0,
										70, kFBAttachNone, NULL, 1.0);

	AddRegion("DTWWarpAmountLabel", "DTWWarpAmountLabel",
										lS, kFBAttachLeft, "ExecuteDTWRegion", 1.0,
										lH, kFBAttachTop, "ExecuteDTWRegion", 1.0,
										45, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("DTWWarpPercentVal", "DTWWarpPercentVal",
										lS, kFBAttachRight, "DTWWarpAmountLabel", 1.0,
										lH, kFBAttachTop, "ExecuteDTWRegion", 1.0,
										45, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);	

	AddRegion("DTWWarpFreqLabel", "DTWWarpFreqLabel",
										lS, kFBAttachRight, "DTWWarpPercentVal", 1.0,
										lH, kFBAttachTop, "ExecuteDTWRegion", 1.0,
										58, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("DTWWarpFreqVal", "DTWWarpFreqVal",
										lS, kFBAttachRight, "DTWWarpFreqLabel", 1.0,
										lH, kFBAttachTop, "ExecuteDTWRegion", 1.0,
										45, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("DTWWriteLogBtn", "DTWWriteLogBtn",
										lS, kFBAttachLeft, "", 1.0,
										lS, kFBAttachBottom, "DTWWarpAmountLabel", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("ButtonRunPredictiveDTW", "ButtonRunPredictiveDTW",
										lS, kFBAttachRight, "DTWWriteLogBtn", 1.0,
										lS, kFBAttachBottom, "DTWWarpAmountLabel", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	AddRegion("ButtonRunStandardDTW", "ButtonRunStandardDTW",
										lS, kFBAttachRight, "ButtonRunPredictiveDTW", 1.0,
										lS, kFBAttachBottom, "DTWWarpAmountLabel", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);
	
	SetControl("ExecuteDTWRegion", mDTWRegion);
	SetControl("DTWWarpAmountLabel", mDTWWarpAmountLabel);
	SetControl("DTWWarpPercentVal", mDTWWarpPercentVal);
	SetControl("DTWWarpFreqLabel", mDTWWarpFreqLabel);
	SetControl("DTWWarpFreqVal", mDTWWarpFreqVal);
	SetControl("DTWWriteLogBtn", mDTWLogBtn);
	SetControl("ButtonRunStandardDTW", mRunStandardDTWBtn);
	SetControl("ButtonRunPredictiveDTW", mRunPredictiveDTWBtn);

	// define export time warp curves region
	AddRegion("ExportCurvesRegion", "ExportCurvesRegion", 
										lS, kFBAttachLeft, "", 1.0,
										lH, kFBAttachBottom, "ExecuteDTWRegion", 1.0,
										lW, kFBAttachNone, "", 1.0,
										50, kFBAttachNone, NULL, 1.0);

	AddRegion("ButtonExportCurves", "ButtonExportCurves",
										lS, kFBAttachLeft, "ExportCurvesRegion", 1.0,
										lH, kFBAttachTop, "ExportCurvesRegion", 1.0,
										lBtnW, kFBAttachNone, "", 1.0,
										lH, kFBAttachNone, NULL, 1.0);

	SetControl("ExportCurvesRegion", mExportCurvesRegion);
	SetControl("ButtonExportCurves", mExportCurvesBtn);

}

/************************************************
 *	UI configuration.
 ************************************************/
void ORToolDTWManager::UIConfigure()
{
	// set-up import region
	SetRegionTitle("ImportRegion", "Select and Import Motion");
	SetBorder("ImportRegion", kFBStandardSmoothBorder, true, true, 2, 2, 90, 0); // set border style

	mMotionList.Style = kFBDropDownList; // set type of list

	mImportMotionBtn.Caption = "Import Motion";
	mImportMotionBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventImportMotionClick);

	// set-up uniform warp region
	SetRegionTitle("UniformWarpRegion", "Apply Uniform Warp");
	SetBorder("UniformWarpRegion", kFBStandardSmoothBorder, true, true, 2, 2, 90, 0); // set borader style

	mUniformWarpTypeLabel.Caption = "Type:";
	mUniformWarpTypeLabel.Justify = kFBTextJustifyRight;

	mWarpList.Style = kFBDropDownList;  // set type of list

	mUniformWarpPercentLabel.Caption = "Milliseconds:";
	mUniformWarpPercentLabel.Justify = kFBTextJustifyRight;

	mUniformWarpPercentVal.Value = 0; 
	mUniformWarpPercentVal.Min = 0;
	mUniformWarpPercentVal.Max = 1000;

	mWarpBtn.Caption = "Uniform Warp";
	mWarpBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonPerformWarp); // set button callback

	// random warp region
	SetRegionTitle("RandomWarpRegion", "Apply Random Warp");
	SetBorder("RandomWarpRegion", kFBStandardSmoothBorder, true, true, 2, 2, 90, 0); // set borader style

	mGenerateRandWarpBtn.Caption = "Generate Values";
	mGenerateRandWarpBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonGenerateRandomWarp); // set button callback

	mSaveRandWarpBtn.Caption = "Save Values";
	mSaveRandWarpBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonSaveRandomWarp);

	mOpenRandWarpBtn.Caption = "Open Values";
	mOpenRandWarpBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonOpenRandomWarp);
	
	mApplyRandWarpBtn.Caption = "Apply Values";
	mApplyRandWarpBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonApplyRandomWarp); // set button callback

	// dynamic timewarp button region
	SetRegionTitle("ExecuteDTWRegion", "Dynamic Time Warp");
	SetBorder("ExecuteDTWRegion", kFBStandardSmoothBorder, true, true, 2, 2, 90, 0); // set border style

	mDTWWarpAmountLabel.Caption = "Warp Percent:";
	mDTWWarpAmountLabel.Justify = kFBTextJustifyRight;

	mDTWWarpPercentVal.Value = 5.0;
	mDTWWarpPercentVal.Min = 0.0;
	mDTWWarpPercentVal.Max = 100.0;

	mDTWWarpFreqLabel.Caption = "Frequency:";
	mDTWWarpFreqLabel.Justify = kFBTextJustifyRight;

	mDTWWarpFreqVal.Value = 25;
	mDTWWarpFreqVal.Min = 1;
	mDTWWarpFreqVal.Max = 200;

	mDTWLogBtn.Caption = "Output Log:";
	mDTWLogBtn.Style = kFBCheckbox;

	//cout << "buttonnState:" << mDTWLogBtn.State << endl;
	
	mRunPredictiveDTWBtn.Caption = "Run Predictive DTW...";
	mRunPredictiveDTWBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonRunPredictiveDTW); // set button call back

	mRunStandardDTWBtn.Caption = "Run Standard DTW...";
	mRunStandardDTWBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonRunStandardDTW); // set button call back

	// set-up export timewarp curves region
	SetRegionTitle("ExportCurvesRegion", "Export Time Warp Curves");
	SetBorder("ExportCurvesRegion", kFBStandardSmoothBorder, true, true, 2, 2, 90, 0); // set border style

	mExportCurvesBtn.Caption = "Export Curves...";
	mExportCurvesBtn.OnClick.Add(this, (FBCallback)&ORToolDTWManager::EventButtonExportCurves); // set button call back

	// clear the motion list
	mMotionList.Items.Clear();

	// populate the motionlist with the names of the motions
	pugi::xml_parse_result lresult = mXMLDoc.load_file(mMotionListXMLFile);

	for (pugi::xml_node lmotion = mXMLDoc.child("Motion_Library").child("Motion"); lmotion; lmotion = lmotion.next_sibling("Motion"))
	{
		mMotionList.Items.Add(lmotion.child_value("Name"));
	}

	// add warp options to list
	mWarpList.Items.Clear();
	mWarpList.Items.Add("None");
	mWarpList.Items.Add("Stretch");
	mWarpList.Items.Add("Shrink");
	mWarpList.Items.Add("Fast then slow");
	mWarpList.Items.Add("Slow then fast");
	mWarpList.Items.Add("Slow - fast - slow");
	mWarpList.Items.Add("Fast - slow - fast");

	//configure list for random warp parameters
	mRandomWarpDisp.Caption = "Warp configuration";
	mRandomWarpDisp.ColumnAdd("Warp Type"); // add columns
	mRandomWarpDisp.ColumnAdd("Warp Value");
	mRandomWarpDisp.GetColumn(0).Width = 100; // set column widths
	mRandomWarpDisp.GetColumn(1).Width = 100;

	mRandomWarpDisp.GetColumn(1).Style = FBCellStyle::kFBCellStyleInteger;  // set cells in second column to integer
	for (int i = 0; i < mNumRandomWarpEffectors; i++){
		mRandomWarpDisp.RowAdd(EffectorLookup(mRandomWarpEffectorList[i]), i); //add row for each effector that warps are being created for, last param acts as a reference
	}

}

/************************************************
 *	Handle tool activation (selection/unselection).
 ************************************************/
void ORToolDTWManager::UIReset()
{


}

/************************************************
*	Pupulates the random warp display with current
************************************************/
void ORToolDTWManager::PopulateRandomWarpDisp()
{
	// loop through each effctor effected by warp
	for (int i = 0; i < mNumRandomWarpEffectors; i++){
		mRandomWarpDisp.SetCell(i, 0, mWarpList.Items[mRandomWarpList[i].warpType]);
		mRandomWarpDisp.SetCell(i, 1, mRandomWarpList[i].warpValue);
	}
}

/************************************************
*	Effector lookup, gives the name of an effector when given an effector ID
************************************************/
const char* ORToolDTWManager::EffectorLookup(int pEffectorID)
{
	switch (pEffectorID)
	{
		case 15:
			return "Head Effector";
			break;
		case 3:
			return "Left Wrist Effector";
			break;
		case 4:
			return "Right Wrist Effector";
			break;
		case 1:
			return "Left Ankle Effector";
			break;
		case 2:
			return "Right Angle Effector";
			break;
		default:
			return "Not in range";
			break;
	}
}

/************************************************
*	Determines the length of animation on character rig by finding the last curve key on all effectors
************************************************/
FBTime ORToolDTWManager::FindLastKey(FBCharacter* pCharacter){

	//set variables
	FBModel* lEffector;  // effector being checked in loop
	FBAnimationNode* lAnimNode; // animation node of effector being checked
	FBTime lCurveTime; // length of curve for effector being checked
	
	FBTime lTimeOfLastKey;  // contain the length of largest curve found
	lTimeOfLastKey.SetMilliSeconds(0);  // set zero to start

	// llop through each effector
	for (int i = 0; i < mNumEffectors; i++){

		// get effector model and animation node 
		lEffector = pCharacter->GetEffectorModel(FBEffectorId(mEffectorEnumList[i]));
		lAnimNode = lEffector->Translation.GetAnimationNode();
		
		// get the time of the last animation node
		lCurveTime = lAnimNode->Nodes[0]->FCurve->Keys[lAnimNode->Nodes[0]->FCurve->Keys.GetCount() - 1].Time;

		// if curve is longer than curves already checked then wright of lest key position
		if (lCurveTime > lTimeOfLastKey) {
			lTimeOfLastKey = lCurveTime;
		}
	}

	return lTimeOfLastKey;

}


/**********************************************
*   Recursively select every model and descendents
***********************************************/
void ORToolDTWManager::SelectBranch(FBModel* pTopModel)
{
	FBPropertyListModel lChildren = pTopModel->Children;
	for (int i = 0; i < lChildren.GetCount(); i++){
		FBModel* lChildModel = lChildren[i];
		lChildModel->Selected = true;
		SelectBranch(lChildModel);
	}
	pTopModel->Selected = true;
}

/**********************************************
*   Connect Chracater to BHV Motion file
***********************************************/
void ORToolDTWManager::ConnectCharacterToMotion(FBCharacter* pCharacter, FBModel* pTopModel)
{
	// get a list of children of the model
	FBPropertyListModel lChildren = pTopModel->Children;

	// for each child
	for (int i = 0; i < lChildren.GetCount(); i++){
		FBModel* lChildModel = lChildren[i];

		MapObjectToCharacter(pCharacter, lChildModel);
		ConnectCharacterToMotion(pCharacter, lChildModel);
	}
	
}

/**********************************************
*   Map object to character
***********************************************/
void ORToolDTWManager::MapObjectToCharacter(FBCharacter* pCharacter, FBModel* pObject)
{
	string lPropName = string(pObject->Name) + "Link";
	FBProperty* lProp = pCharacter->PropertyList.Find(lPropName.c_str());
	if (lProp)
	{
		lProp->ConnectSrc(pObject);
	}
}



/**********************************************
*   Change a property of all nodes in a skeleton
***********************************************/
void ORToolDTWManager::SetPropForAllNodes(FBModel* pTopModel, const char* pName, double pValue)
{
	// get a list of children of the model
	FBPropertyListModel lChildren = pTopModel->Children;

	// for each child
	for (int i = 0; i < lChildren.GetCount(); i++){
		FBModel* lChildModel = lChildren[i];
		SetProp(lChildModel, pName, pValue);  // set prop for each children of the model passed in this function
		SetPropForAllNodes(lChildModel, pName, pValue);  // recursive call on function to edit hte prop of children of this model.
	}
	SetProp(pTopModel, pName, pValue); //set prop of the model passed in this function
}

void ORToolDTWManager::SetPropForAllNodes(FBModel* pTopModel, const char* pName, FBColor* pValue)
{
	// get a list of children of the model
	FBPropertyListModel lChildren = pTopModel->Children;

	// for each child
	for (int i = 0; i < lChildren.GetCount(); i++){
		FBModel* lChildModel = lChildren[i];
		SetProp(lChildModel, pName, pValue);  // set prop for each children of the model passed in this function
		SetPropForAllNodes(lChildModel, pName, pValue);  // recursive call on function to edit hte prop of children of this model.
	}
	SetProp(pTopModel, pName, pValue); //set prop of the model passed in this function
}

/***********************************************
*   Set the property of node for indivual model
************************************************/
void ORToolDTWManager::SetProp(FBModel* pModel, const char* pName, double pValue)
{
	//find property
	FBProperty* lProp = pModel->PropertyList.Find(pName);
	
	// set property
	if (lProp){
		lProp->SetData(&pValue);

	}
	
}


void ORToolDTWManager::SetProp(FBModel* pModel, const char* pName, FBColor* pValue)
{
	//find property
	FBProperty* lProp = pModel->PropertyList.Find(pName);

	// set property
	if (lProp){
		lProp->SetData(pValue);

	}

}


/***********************************************
*     List all properties in a porerty list
************************************************/

void ORToolDTWManager::ListProperties(FBPropertyManager pPropManager)
{
	FBProperty* lProp;
	for (int i = 0; i < pPropManager.GetCount(); i++){
		lProp = pPropManager[i];
		cout << lProp->GetName() << ": " << lProp->GetPropertyTypeName() << endl;
	}
	cout << endl;
}

/*****************************************
*     add all the animated children of a model to a timewarp
*************************************/
void ORToolDTWManager::addAllNodesToWarp(FBModel* pTopModel, FBAnimationNode* pTimeWarp)
{
	//setup local variables
	FBModel* lChildModel;
	FBAnimationNode* lAnimNode;
	
	// get a list of children of the model
	FBPropertyListModel lChildren = pTopModel->Children;

	// for each child
	for (int i = 0; i < lChildren.GetCount(); i++){
		lChildModel = lChildren[i];

		// apply warp to rotation
		lAnimNode = lChildModel->Rotation.GetAnimationNode();
		FBTimeWarpManager::TheOne().ApplyTimeWarp(mSystem.CurrentTake, &lChildModel->Rotation, pTimeWarp);
		FBTimeWarpManager::TheOne().TimeWarpMergeCurveNode(mSystem.CurrentTake, &lChildModel->Rotation, lAnimNode, pTimeWarp);

		// applywarp to transaltion
		lAnimNode = lChildModel->Translation.GetAnimationNode();
		FBTimeWarpManager::TheOne().ApplyTimeWarp(mSystem.CurrentTake, &lChildModel->Rotation, pTimeWarp);
		FBTimeWarpManager::TheOne().TimeWarpMergeCurveNode(mSystem.CurrentTake, &lChildModel->Rotation, lAnimNode, pTimeWarp);

		// perform action recursively
		addAllNodesToWarp(lChildModel, pTimeWarp);
	}

}

/*********************************************
*      Create a stretch warp curve
**********************************************/
void ORToolDTWManager::StretchWarp(FBFCurve* pCurve, double pValue){
	
	//determine new take dureation
	FBTime lWarpedTakeTime = mTimespan.GetStop() * (1 + (pValue / 100));

	//stretch the warp curve
	pCurve->Keys[1].Time = lWarpedTakeTime;
	
}


/*********************************************
*      Create a shrink warp curve
**********************************************/
void ORToolDTWManager::ShrinkWarp(FBFCurve* pCurve, double pValue){

	//determine new take dureation
	FBTime lWarpedTakeTime = mTimespan.GetStop() * (1 - (pValue / 100));

	//stretch the warp curve
	pCurve->Keys[1].Time = lWarpedTakeTime;

}

/*********************************************
*      Create a fast in slow out warp curve - converted to milliseconds
**********************************************/
void ORToolDTWManager::FastSlowWarp(FBFCurve* pCurve, double pValue){

	// determine the mid point of the take
	FBTime lMiddleOfTake = mTimespan.GetStop() / 2;

	// get value of final time warp curve for reference
	double lEndWarpVal = pCurve->Keys[pCurve->Keys.GetCount() - 1].Value;

	// determine the value of the midpoint key 1 = normal position
	double lKeyVal = (lEndWarpVal / 2) + (pValue / 1000);

	// add new key to create warp
	pCurve->KeyAdd(lMiddleOfTake, lKeyVal);
	
}

/*********************************************
*      Create a slow in fast out warp curve
**********************************************/
void ORToolDTWManager::SlowFastWarp(FBFCurve* pCurve, double pValue){

	// determine the mid point of the take
	FBTime lMiddleOfTake = mTimespan.GetStop() / 2;

	// get value of final time warp curve for reference
	double lEndWarpVal = pCurve->Keys[pCurve->Keys.GetCount() - 1].Value;

	// determine the value of the midpoint key 1 = normal position
	double lKeyVal = (lEndWarpVal / 2) * (1 - (pValue / 100));

	// add new key to create warp
	pCurve->KeyAdd(lMiddleOfTake, lKeyVal);

}

/*********************************************
*      Create a slow in slow out warp
**********************************************/
void ORToolDTWManager::SlowInSlowOutWarp(FBFCurve* pCurve, double pValue){

	// determine points to add key frames, thirds along take
	FBTime lKeyTime1 = mTimespan.GetStop() / 3;
	FBTime lKeyTime2 = lKeyTime1 * 2;

	// get value of final time warp curve for reference
	double lEndWarpVal = pCurve->Keys[pCurve->Keys.GetCount() - 1].Value;

	// determine the value of the midpoint key 1 = normal position
	double lKeyVal1 = (lEndWarpVal / 3) * (1 - (pValue / 100));
	double lKeyVal2 = ((lEndWarpVal / 3) * 2) * (1 + (pValue / 100));


	// add new key to create warp
	pCurve->KeyAdd(lKeyTime1, lKeyVal1);
	pCurve->KeyAdd(lKeyTime2, lKeyVal2);

}

/*********************************************
*      Create a fast in fast out warp
**********************************************/
void ORToolDTWManager::FastInFastOutWarp(FBFCurve* pCurve, double pValue){

	// determine points to add key frames, thirds along take
	FBTime lKeyTime1 = mTimespan.GetStop() / 3;
	FBTime lKeyTime2 = lKeyTime1 * 2;

	// get value of final time warp curve for reference
	double lEndWarpVal = pCurve->Keys[pCurve->Keys.GetCount() - 1].Value;

	// determine the value of the midpoint key 1 = normal position
	double lKeyVal1 = (lEndWarpVal / 3) * (1 + (pValue / 100));
	double lKeyVal2 = ((lEndWarpVal / 3) * 2) * (1 - (pValue / 100));


	// add new key to create warp
	pCurve->KeyAdd(lKeyTime1, lKeyVal1);
	pCurve->KeyAdd(lKeyTime2, lKeyVal2);

}

/*********************************************
*      Convert the first key of a curve to a cubic interpolation
**********************************************/
void ORToolDTWManager::SetFirstKeyToCubic(FBFCurve* pCurve){

	// turn on cubic interpolation
	pCurve->Keys[0].Interpolation = FBInterpolation::kFBInterpolationCubic;

	// Set the tengent to a smooth position using the slop between key and next key
	pCurve->Keys[0].TangentMode = FBTangentMode::kFBTangentModeTimeIndependent;
}


/***********************************************
*     Import a motion file
************************************************/
FBCharacter* ORToolDTWManager::ImportMotion(const char* pFilename, vector<string> &pListOfNodesToMap, const char* pName){
	 
	 // import the motion file
	 mApp.FileImport(pFilename, false, true);
	 
	 //rename namespace
	 mSystem.Scene->NamespaceRename("BVH", pName, true);

	 //find reference node for model
	 string lLabel = string(pName) + string(":reference"); // determine model label
	 FBModel* lModel = FBFindModelByLabelName(lLabel.c_str());  // find model

	 // scale the model
	 lModel->Scaling = { 6.0, 6.0, 6.0 };

	 // scale nodes
	 SetPropForAllNodes(lModel, "Size", 30.0);

	 // create character
	 FBCharacter* lCharacter = new FBCharacter(pName);

	 // map character nodes to motion nodes
	 for (int i = 0; i < pListOfNodesToMap.size(); i++){
		 lLabel = string(pName) + ":" + pListOfNodesToMap[i];
		 lModel = FBFindModelByLabelName(lLabel.c_str());
		 MapObjectToCharacter(lCharacter, lModel);
	 }

	 // characterize model as biped
	 lCharacter->SetCharacterizeOn(true);

	 // create control rig
	 lCharacter->CreateControlRig(true);

	 // label control rig
	 FBControlSet* lControlSet = lCharacter->GetCurrentControlSet();
	 lLabel = string(pName) + "_Rig";
	 lControlSet->Name = lLabel.c_str();

	 // plot to control rig and skeleton
	 FBPlotOptions* lPlotOptions = new FBPlotOptions();
	 lCharacter->PlotAnimation(FBCharacterPlotWhere::kFBCharacterPlotOnControlRig, lPlotOptions);
	 lCharacter->PlotAnimation(FBCharacterPlotWhere::kFBCharacterPlotOnSkeleton, lPlotOptions);

	 //activateControl Rig
	 lCharacter->ActiveInput = true;

	 return lCharacter;

}

/***********************************************
*     Label Character
************************************************/
void ORToolDTWManager::labelCharacter(FBCharacter* pCharacter, const char* pName){
	
	// get reference and position of hips effector in control rig
	FBModel* lHipsModel = pCharacter->GetCtrlRigModel(FBBodyNodeId::kFBHipsNodeId);
	FBVector3d lHipsPos = EvaluateNodes(lHipsModel->Translation.GetAnimationNode(), mSystem.LocalTime);

	// get reference and position od reference node of control rig
	FBModel* lRefModel = pCharacter->GetCtrlRigModel(FBBodyNodeId::kFBReferenceNodeId);
	FBVector3d lRefPos = lRefModel->Translation;

	// create a Marker
	FBModelMarker* lMarker = new FBModelMarker(pName); 
	lMarker->Show = true;

	// positon the cube ove hips effector taking account of offset by reference node 
	FBVector3d lMarkerPos = add3dVect(lHipsPos, lRefPos);
	lMarkerPos[1] = 170;  // put cube above hips node
	lMarker->Translation = lMarkerPos;

	// set the marker to display
	FBProperty* lProp = lMarker->PropertyList.Find("ShowLabel");
	bool lPropVal = true;
	lProp->SetData(&lPropVal);

	// create a position constraint
	FBConstraint* lConstraint = FBConstraintManager::TheOne().TypeCreateConstraint("Position");

	// define the source and constraint models for the constraint
	lConstraint->ReferenceAdd(0, lMarker);
	lConstraint->ReferenceAdd(1, lHipsModel);

	// freeze the current differnce in position.
	lConstraint->FreezeSRT(lMarker, false, false, true);

	// activate the constraint.
	lConstraint->Active = true;

}

/************************************************
*	Setup Animation Nodes for DTW waarping and set the weighting of the manipulation 100%
************************************************/
void ORToolDTWManager::SetUpAnimatonNodes(void){

	// set up list of effector models for both orginal and warped characters
	mWarpedEffectorModels = new FBModel*[mNumEffectors];
	mOriginalEffectorModels = new FBModel*[mNumEffectors];

	//set-up list of animation nodes and is animated for both orginal and warped characters
	mWarpedEffectorTransAnimNodes = new FBAnimationNode*[mNumEffectors];
	mOriginalEffectorTransAnimNodes = new FBAnimationNode*[mNumEffectors];
	mOriginalEffectorRotationAnimNodes = new FBAnimationNode*[mNumEffectors];

	// value for IK T Blend
	double lIKBlendVal = 100;

	// for each effector
	for (int i = 0; i < mNumEffectors; i++){

		// store reference for effector in Orginal and Warped model
		mWarpedEffectorModels[i] = mWarpedCharacter->GetEffectorModel(FBEffectorId(mEffectorEnumList[i]));
		mOriginalEffectorModels[i] = mOrginalCharacter->GetEffectorModel(FBEffectorId(mEffectorEnumList[i]));

		// check if and store if effector is animated on warped model
		if (mWarpedEffectorModels[i]->Translation.IsAnimated() == true){
			mWarpedEffectorTransAnimNodes[i] = mWarpedEffectorModels[i]->Translation.GetAnimationNode();
		}

		// check if and store if effector is animated on translation of original model
		if (mOriginalEffectorModels[i]->Translation.IsAnimated() == true){
			mOriginalEffectorTransAnimNodes[i] = mOriginalEffectorModels[i]->Translation.GetAnimationNode();
		}

		// check if and store if effector is animated on rotation of original model
		if (mOriginalEffectorModels[i]->Rotation.IsAnimated() == true){
			mOriginalEffectorRotationAnimNodes[i] = mOriginalEffectorModels[i]->Rotation.GetAnimationNode();
		}

		// set the IK T Blend value to 100 for the effector
		FBProperty* lProp = mOriginalEffectorModels[i]->PropertyList.Find("IK Reach Translation");
		lProp->SetData(&lIKBlendVal);
		lProp = mOriginalEffectorModels[i]->PropertyList.Find("IK Reach Rotation");
		lProp->SetData(&lIKBlendVal);
	}

}


/***********************************************
*     get the value of animation nodes at a particular time and store them in an array
************************************************/
FBVector3d ORToolDTWManager::EvaluateNodes(FBAnimationNode* pAnimNode, FBTime pTime){

	FBVector3d lVector;

	for (int i = 0; i < 3; i++){

		lVector[i] = pAnimNode->Nodes[i]->FCurve->Evaluate(pTime);

	}
	
	return lVector;

}


/***********************************************
*    Predict frame
************************************************/
FBVector3d  ORToolDTWManager::lPredictPos(FBVector3d pPrevFramePos, FBVector3d pCurrentFramePos){

	FBVector3d lVector;

	for (int i = 0; i < 3; i++){

		lVector[i] = pCurrentFramePos[i] + (pCurrentFramePos[i] - pPrevFramePos[i]);

	}

	return lVector;

}

/***********************************************
*    Add key frames to all 3 xyz nodes using 3DVector and FBTime
************************************************/
void ORToolDTWManager::add3DVectorKey(FBAnimationNode* pAnimNode, FBVector3d pVector, FBTime pTime){

	for (int i = 0; i < 3; i++){
		pAnimNode->Nodes[i]->KeyAdd(pTime, &pVector[i]);
	}

}

/***********************************************
*    calculates the distance between two 3d vectors
************************************************/

double ORToolDTWManager::calcDistance3d(FBVector3d pV1, FBVector3d pV2){
	double lSum = 0;
	for (int i = 0; i < 3; i++){
		lSum += pow(pV1[i] - pV2[i], 2);
	}
	return sqrt(lSum);
}

/***********************************************
*    add two 3D vectors
************************************************/

FBVector3d ORToolDTWManager::add3dVect(FBVector3d pV1, FBVector3d pV2){
	
	FBVector3d lVector;

	// loop through each the numbers in each vector adding them together
	for (int i = 0; i < 3; i++){
		lVector[i] = pV1[i] + pV2[i];
	}
	return lVector;
}