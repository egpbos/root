// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006 - 2009

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//==============================================================================
// LinkDef1.h - Core EVE objects and services.
//==============================================================================

// REveVector
#pragma link C++ class   ROOT::Experimental::REveVectorT<Float_t>+;
#pragma link C++ class   ROOT::Experimental::REveVectorT<Double_t>+;
#pragma link C++ typedef ROOT::Experimental::REveVector;
#pragma link C++ typedef ROOT::Experimental::REveVectorF;
#pragma link C++ typedef ROOT::Experimental::REveVectorD;

#pragma link C++ class   ROOT::Experimental::REveVector4T<Float_t>+;
#pragma link C++ class   ROOT::Experimental::REveVector4T<Double_t>+;
#pragma link C++ typedef ROOT::Experimental::REveVector4;
#pragma link C++ typedef ROOT::Experimental::REveVector4F;
#pragma link C++ typedef ROOT::Experimental::REveVector4D;

#pragma link C++ class   ROOT::Experimental::REveVector2T<Float_t>+;
#pragma link C++ class   ROOT::Experimental::REveVector2T<Double_t>+;
#pragma link C++ typedef ROOT::Experimental::REveVector2;
#pragma link C++ typedef ROOT::Experimental::REveVector2F;
#pragma link C++ typedef ROOT::Experimental::REveVector2D;

// Operators for REveVectorXT<Float_t>
#pragma link C++ function operator+(const ROOT::Experimental::REveVectorT<Float_t>&, const ROOT::Experimental::REveVectorT<Float_t>&);
#pragma link C++ function operator-(const ROOT::Experimental::REveVectorT<Float_t>&, const ROOT::Experimental::REveVectorT<Float_t>&);
#pragma link C++ function operator*(const ROOT::Experimental::REveVectorT<Float_t>&, Float_t);
#pragma link C++ function operator*(Float_t, const ROOT::Experimental::REveVectorT<Float_t>&);
#pragma link C++ function operator+(const ROOT::Experimental::REveVector4T<Float_t>&, const ROOT::Experimental::REveVector4T<Float_t>&);
#pragma link C++ function operator-(const ROOT::Experimental::REveVector4T<Float_t>&, const ROOT::Experimental::REveVector4T<Float_t>&);
#pragma link C++ function operator*(const ROOT::Experimental::REveVector4T<Float_t>&, Float_t);
#pragma link C++ function operator*(Float_t, const ROOT::Experimental::REveVector4T<Float_t>&);
#pragma link C++ function operator+(const ROOT::Experimental::REveVector2T<Float_t>&, const ROOT::Experimental::REveVector2T<Float_t>&);
#pragma link C++ function operator-(const ROOT::Experimental::REveVector2T<Float_t>&, const ROOT::Experimental::REveVector2T<Float_t>&);
#pragma link C++ function operator*(const ROOT::Experimental::REveVector2T<Float_t>&, Float_t);
#pragma link C++ function operator*(Float_t, const ROOT::Experimental::REveVector2T<Float_t>&);
// Operators for REveVectorXT<Double_t>
#pragma link C++ function operator+(const ROOT::Experimental::REveVectorT<Double_t>&, const ROOT::Experimental::REveVectorT<Double_t>&);
#pragma link C++ function operator-(const ROOT::Experimental::REveVectorT<Double_t>&, const ROOT::Experimental::REveVectorT<Double_t>&);
#pragma link C++ function operator*(const ROOT::Experimental::REveVectorT<Double_t>&, Double_t);
#pragma link C++ function operator*(Double_t, const ROOT::Experimental::REveVectorT<Double_t>&);
#pragma link C++ function operator+(const ROOT::Experimental::REveVector4T<Double_t>&, const ROOT::Experimental::REveVector4T<Double_t>&);
#pragma link C++ function operator-(const ROOT::Experimental::REveVector4T<Double_t>&, const ROOT::Experimental::REveVector4T<Double_t>&);
#pragma link C++ function operator*(const ROOT::Experimental::REveVector4T<Double_t>&, Double_t);
#pragma link C++ function operator*(Double_t, const ROOT::Experimental::REveVector4T<Double_t>&);
#pragma link C++ function operator+(const ROOT::Experimental::REveVector2T<Double_t>&, const ROOT::Experimental::REveVector2T<Double_t>&);
#pragma link C++ function operator-(const ROOT::Experimental::REveVector2T<Double_t>&, const ROOT::Experimental::REveVector2T<Double_t>&);
#pragma link C++ function operator*(const ROOT::Experimental::REveVector2T<Double_t>&, Double_t);
#pragma link C++ function operator*(Double_t, const ROOT::Experimental::REveVector2T<Double_t>&);

// REvePathMark
#pragma link C++ class   ROOT::Experimental::REvePathMarkT<Float_t>+;
#pragma link C++ class   ROOT::Experimental::REvePathMarkT<Double_t>+;
#pragma link C++ typedef ROOT::Experimental::REvePathMark;
#pragma link C++ typedef ROOT::Experimental::REvePathMarkF;
#pragma link C++ typedef ROOT::Experimental::REvePathMarkD;

// REveTrans
#pragma link C++ class ROOT::Experimental::REveTrans-;

// REveUtil
#pragma link C++ class ROOT::Experimental::REveUtil+;
#pragma link C++ class ROOT::Experimental::REveException+;
#pragma link C++ class ROOT::Experimental::REvePadHolder+;
#pragma link C++ class ROOT::Experimental::REveGeoManagerHolder+;
#pragma link C++ class ROOT::Experimental::REveRefCnt+;
#pragma link C++ class ROOT::Experimental::REveRefBackPtr+;

// REveManager
#pragma link C++ class ROOT::Experimental::REveManager+;
#pragma link C++ global ROOT::Experimental::gEve;
#pragma link C++ class ROOT::Experimental::REveManager::RRedrawDisabler+;
#pragma link C++ class ROOT::Experimental::REveManager::RExceptionHandler+;

// REveVSD
#pragma link C++ class ROOT::Experimental::REveMCTrack+;
#pragma link C++ class ROOT::Experimental::REveHit+;
#pragma link C++ class ROOT::Experimental::REveCluster+;

#pragma link C++ class   ROOT::Experimental::REveRecTrackT<Float_t>+;
#pragma link C++ class   ROOT::Experimental::REveRecTrackT<Double_t>+;
#pragma link C++ typedef ROOT::Experimental::REveRecTrack;
#pragma link C++ typedef ROOT::Experimental::REveRecTrackF;
#pragma link C++ typedef ROOT::Experimental::REveRecTrackD;

#pragma link C++ class ROOT::Experimental::REveRecKink+;
#pragma link C++ class ROOT::Experimental::REveRecV0+;
#pragma link C++ class ROOT::Experimental::REveRecCascade+;
#pragma link C++ class ROOT::Experimental::REveMCRecCrossRef+;

#pragma link C++ class ROOT::Experimental::REveVSD+;

// REveTreeTools
#pragma link C++ class ROOT::Experimental::REveSelectorToEventList+;
#pragma link C++ class ROOT::Experimental::REvePointSelectorConsumer+;
#pragma link C++ class ROOT::Experimental::REvePointSelector+;

// REveElement
#pragma link C++ class ROOT::Experimental::REveRenderData+;
#pragma link C++ class ROOT::Experimental::REveElement+;
#pragma link C++ class ROOT::Experimental::REveElementObjectPtr+;
#pragma link C++ class ROOT::Experimental::REveElementList+;
#pragma link C++ class ROOT::Experimental::REveElementListProjected+;

#pragma link C++ class std::list<ROOT::Experimental::REveElement*>;
#pragma link C++ class std::list<ROOT::Experimental::REveElement*>::iterator;
#pragma link C++ class std::list<ROOT::Experimental::REveElement*>::const_iterator;
#pragma link C++ typedef ROOT::Experimental::REveElement::List_t;
#pragma link C++ typedef ROOT::Experimental::REveElement::List_i;
#pragma link C++ typedef ROOT::Experimental::REveElement::List_ci;

#pragma link C++ class std::set<ROOT::Experimental::REveElement*>;
#pragma link C++ class std::set<ROOT::Experimental::REveElement*>::iterator;
#pragma link C++ class std::set<ROOT::Experimental::REveElement*>::const_iterator;
#pragma link C++ typedef ROOT::Experimental::REveElement::Set_t;
#pragma link C++ typedef ROOT::Experimental::REveElement::Set_i;
#pragma link C++ typedef ROOT::Experimental::REveElement::Set_ci;

// REveCompound
#pragma link C++ class ROOT::Experimental::REveCompound+;
#pragma link C++ class ROOT::Experimental::REveCompoundProjected+;

// REveSelection
#pragma link C++ class ROOT::Experimental::REveSelection+;
#pragma link C++ class ROOT::Experimental::REveSecondarySelectable+;

// 3D Viewers and Scenes
#pragma link C++ class ROOT::Experimental::REveScene+;
#pragma link C++ class ROOT::Experimental::REveSceneList+;
#pragma link C++ class ROOT::Experimental::REveSceneInfo+;
#pragma link C++ class ROOT::Experimental::REveViewer+;
#pragma link C++ class ROOT::Experimental::REveViewerList+;

// Data classes
#pragma link C++ class ROOT::Experimental::REveDataCollection+;
#pragma link C++ class ROOT::Experimental::REveDataItem+;
#pragma link C++ class ROOT::Experimental::REveDataTable+;
#pragma link C++ class ROOT::Experimental::REveDataColumn+;

// Projections / non-linear transformations
#pragma link C++ class ROOT::Experimental::REveProjectable+;
#pragma link C++ class ROOT::Experimental::REveProjected+;
#pragma link C++ class ROOT::Experimental::REveProjection+;
#pragma link C++ class ROOT::Experimental::REveProjection::PreScaleEntry_t+;
#pragma link C++ class std::vector<ROOT::Experimental::REveProjection::PreScaleEntry_t>;
#pragma link C++ class std::vector<ROOT::Experimental::REveProjection::PreScaleEntry_t>::iterator;
#pragma link C++ operators std::vector<ROOT::Experimental::REveProjection::PreScaleEntry_t>::iterator;
#pragma link C++ typedef ROOT::Experimental::REveProjection::vPreScale_t;
#pragma link C++ typedef ROOT::Experimental::REveProjection::vPreScale_i;
#pragma link C++ class ROOT::Experimental::REveRhoZProjection+;
#pragma link C++ class ROOT::Experimental::REveRPhiProjection+;
#pragma link C++ class ROOT::Experimental::REve3DProjection+;

#pragma link C++ class ROOT::Experimental::REveProjectionManager+;
// #pragma link C++ class ROOT::Experimental::REveProjectionAxes+;

// Generic configuration
// #pragma link C++ class REveParamList;
// #pragma link C++ class REveParamList::FloatConfig_t+;
// #pragma link C++ class REveParamList::IntConfig_t+;
// #pragma link C++ class REveParamList::BoolConfig_t+;

// REveTrack
#pragma link C++ class ROOT::Experimental::REveTrack+;
#pragma link C++ class ROOT::Experimental::REveTrackList+;
#pragma link C++ class ROOT::Experimental::REveTrackProjected+;
#pragma link C++ class ROOT::Experimental::REveTrackListProjected+;

// REveTrackPropagator
#pragma link C++ class ROOT::Experimental::REveTrackPropagator+;
#pragma link C++ class ROOT::Experimental::REveMagField+;
#pragma link C++ class ROOT::Experimental::REveMagFieldConst+;
#pragma link C++ class ROOT::Experimental::REveMagFieldDuo+;

// REvePointSet
#pragma link C++ class ROOT::Experimental::REvePointSet+;
#pragma link C++ class ROOT::Experimental::REvePointSetArray+;
#pragma link C++ class ROOT::Experimental::REvePointSetProjected+;

// REveLine
#pragma link C++ class ROOT::Experimental::REveLine+;
#pragma link C++ class ROOT::Experimental::REveLineProjected+;

// Shapes
#pragma link C++ class ROOT::Experimental::REveShape+;
#pragma link C++ class ROOT::Experimental::REvePolygonSetProjected+;
#pragma link C++ class ROOT::Experimental::REveGeoShape+;
#pragma link C++ class ROOT::Experimental::REveGeoShapeProjected+;
#pragma link C++ class ROOT::Experimental::REveGeoShapeExtract+;
#pragma link C++ class ROOT::Experimental::REveGeoPolyShape+;

// Not yet ported
// #pragma link C++ class ROOT::Experimental::REveGeoNode+;
// #pragma link C++ class ROOT::Experimental::REveGeoTopNode+;

// REveJetCone
#pragma link C++ class ROOT::Experimental::REveJetCone+;
#pragma link C++ class ROOT::Experimental::REveJetConeProjected+;
