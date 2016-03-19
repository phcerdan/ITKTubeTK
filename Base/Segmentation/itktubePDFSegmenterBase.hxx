/*=========================================================================

Library:   TubeTK

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/

#ifndef __itktubePDFSegmenterBase_hxx
#define __itktubePDFSegmenterBase_hxx

#include "itktubePDFSegmenterBase.h"
#include "itktubeVectorImageToListGenerator.h"

#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkConnectedThresholdImageFilter.h>
#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itktubeSmoothingRecursiveGaussianImageFilter.h>
#include <itkThresholdImageFilter.h>
#include <itkNormalizeToConstantImageFilter.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkJoinImageFilter.h>
#include <itkTimeProbesCollectorBase.h>
#include <itkVotingBinaryIterativeHoleFillingImageFilter.h>

#include <vnl/vnl_matrix.h>

#include <limits>

namespace itk
{

namespace tube
{

template< class TImage, unsigned int N, class TLabelMap >
PDFSegmenterBase< TImage, N, TLabelMap >
::PDFSegmenterBase( void )
{
  m_SampleUpToDate = false;
  m_PDFsUpToDate = false;
  m_ClassProbabilityImagesUpToDate = false;

  m_InClassList.clear();
  m_OutClassList = NULL;

  m_FeatureVectorGenerator = NULL;

  m_LabelMap = NULL;

  m_ObjectIdList.clear();
  m_VoidId = std::numeric_limits< LabelMapPixelType >::max();
  m_PDFWeightList.clear();

  m_ErodeRadius = 1;
  m_HoleFillIterations = 1;
  m_ProbabilityImageSmoothingStandardDeviation = 0;
  m_ReclassifyObjectLabels = false;
  m_ReclassifyNotObjectLabels = false;
  m_ForceClassification = false;

  m_ProbabilityImageVector.resize( 0 );

  m_ProgressProcessInfo = NULL;
  m_ProgressFraction = 1.0;
  m_ProgressStart = 0;
}

template< class TImage, unsigned int N, class TLabelMap >
PDFSegmenterBase< TImage, N, TLabelMap >
::~PDFSegmenterBase( void )
{
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::SetProgressProcessInformation( void * processInfo, double fraction,
  double start )
{
  m_ProgressProcessInfo = processInfo;
  m_ProgressFraction = fraction;
  m_ProgressStart = start;
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::ClearObjectIds( void )
{
  m_ObjectIdList.clear();
  m_PDFWeightList.clear();
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::SetObjectId( ObjectIdType objectId )
{
  m_ObjectIdList.clear();
  m_ObjectIdList.push_back( objectId );
  m_PDFWeightList.clear();
  m_PDFWeightList.push_back( 1.0 );
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::AddObjectId( ObjectIdType objectId )
{
  m_ObjectIdList.push_back( objectId );
  m_PDFWeightList.push_back( 1.0 );
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::SetObjectId( const ObjectIdListType objectId )
{
  m_ObjectIdList = objectId;
  if( m_PDFWeightList.size() != m_ObjectIdList.size() )
    {
    m_PDFWeightList.resize( m_ObjectIdList.size(), 1.0 );
    }
}

template< class TImage, unsigned int N, class TLabelMap >
const typename PDFSegmenterBase< TImage, N, TLabelMap >::VectorIntType &
PDFSegmenterBase< TImage, N, TLabelMap >
::GetObjectId( void ) const
{
  return m_ObjectIdList;
}

template< class TImage, unsigned int N, class TLabelMap >
unsigned int
PDFSegmenterBase< TImage, N, TLabelMap >
::GetNumberOfObjectIds( void ) const
{
  return m_ObjectIdList.size();
}

template< class TImage, unsigned int N, class TLabelMap >
unsigned int
PDFSegmenterBase< TImage, N, TLabelMap >
::GetNumberOfClasses( void ) const
{
  return m_ObjectIdList.size();
}

template< class TImage, unsigned int N, class TLabelMap >
unsigned int
PDFSegmenterBase< TImage, N, TLabelMap >
::GetObjectNumberFromId( ObjectIdType id ) const
{
  for( unsigned int i = 0; i < m_ObjectIdList.size(); i++ )
    {
    if( m_ObjectIdList[i] == id )
      {
      return i;
      }
    }
  throw;
}

template< class TImage, unsigned int N, class TLabelMap >
unsigned int
PDFSegmenterBase< TImage, N, TLabelMap >
::GetNumberOfFeatures( void ) const
{
  return N;
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::SetObjectPDFWeight( unsigned int num, double weight )
{
  m_PDFWeightList[num] = weight;
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::SetObjectPDFWeight( const VectorDoubleType & weight )
{
  m_PDFWeightList = weight;
}

template< class TImage, unsigned int N, class TLabelMap >
const typename PDFSegmenterBase< TImage, N, TLabelMap >::VectorDoubleType &
PDFSegmenterBase< TImage, N, TLabelMap >
::GetObjectPDFWeight( void ) const
{
  return m_PDFWeightList;
}

template< class TImage, unsigned int N, class TLabelMap >
typename Image< float, TImage::ImageDimension >::Pointer
PDFSegmenterBase< TImage, N, TLabelMap >
::GetClassProbabilityImage( unsigned int classNum ) const
{
  if( classNum < m_ProbabilityImageVector.size() )
    {
    return m_ProbabilityImageVector[classNum];
    }
  return NULL;
}

template< class TImage, unsigned int N, class TLabelMap >
typename PDFSegmenterBase< TImage, N, TLabelMap >::ProbabilityPixelType
PDFSegmenterBase< TImage, N, TLabelMap >
::GetClassProbability( unsigned int classNum, const FeatureVectorType & fv )
  const
{
  return 0;
}
template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::SetFeatureVectorGenerator( typename FeatureVectorGeneratorType::Pointer
  fvg )
{
  m_FeatureVectorGenerator = fvg;
  m_SampleUpToDate = false;
  m_PDFsUpToDate = false;
  m_ClassProbabilityImagesUpToDate = false;
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::GenerateSample( void )
{
  m_SampleUpToDate = true;

  unsigned int numClasses = m_ObjectIdList.size();

  //
  //  Convert in/out images to statistical list using masks
  //
  m_InClassList.resize( numClasses );
  for( unsigned int c = 0; c < numClasses; c++ )
    {
    m_InClassList[c] = ListSampleType::New();
    }
  m_OutClassList  = ListSampleType::New();

  typedef itk::ImageRegionConstIteratorWithIndex< LabelMapType >
    ConstLabelMapIteratorType;
  ConstLabelMapIteratorType itInLabelMap( m_LabelMap,
    m_LabelMap->GetLargestPossibleRegion() );
  itInLabelMap.GoToBegin();

  ListVectorType v;
  FeatureVectorType fv;
  typename LabelMapType::IndexType indx;
  bool found = false;
  int prevVal = itInLabelMap.Get() + 1;
  int prevC = 0;
  while( !itInLabelMap.IsAtEnd() )
    {
    int val = itInLabelMap.Get();
    bool valid = true;
    indx = itInLabelMap.GetIndex();
    fv = m_FeatureVectorGenerator->GetFeatureVector( indx );
    for( unsigned int i = 0; i < N; i++ )
      {
      v[i] = fv[i];
      }
    for( unsigned int i = 0; i < ImageDimension; i++ )
      {
      v[N+i] = indx[i];
      }
    if( val != prevVal )
      {
      found = false;
      for( unsigned int c = 0; c < numClasses; c++ )
        {
        if( val == m_ObjectIdList[c] )
          {
          found = true;
          prevVal = val;
          prevC = c;
          break;
          }
        }
      }
    if( found )
      {
      m_InClassList[prevC]->PushBack( v );
      }
    else if( !found && val != m_VoidId )
      {
      m_OutClassList->PushBack( v );
      }
    ++itInLabelMap;
    }
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::GeneratePDFs( void )
{
  if( !m_SampleUpToDate )
    {
    this->GenerateSample();
    }
  m_PDFsUpToDate = true;
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::ApplyPDFs( void )
{
  if( m_LabelMap.IsNotNull() && !m_SampleUpToDate )
    {
    this->GenerateSample();
    }
  if( m_LabelMap.IsNotNull() && !m_PDFsUpToDate )
    {
    this->GeneratePDFs();
    }
  m_ClassProbabilityImagesUpToDate = true;

  int erodeRadius = m_ErodeRadius;
  int holeFillIterations = m_HoleFillIterations;

  unsigned int numClasses = m_ObjectIdList.size();

  //
  //  Compute the probability at each pixel for input images
  //
  m_ProbabilityImageVector.resize( numClasses );

  typedef itk::ImageRegionIterator<ProbabilityImageType>
    ProbabilityImageIteratorType;
  std::vector< ProbabilityImageIteratorType * > probIt( numClasses );

  for( unsigned int c = 0; c < numClasses; c++ )
    {
    m_ProbabilityImageVector[c] = ProbabilityImageType::New();
    m_ProbabilityImageVector[c]->SetRegions(
      m_LabelMap->GetLargestPossibleRegion() );
    m_ProbabilityImageVector[c]->CopyInformation( m_LabelMap );
    m_ProbabilityImageVector[c]->Allocate();

    probIt[c] = new ProbabilityImageIteratorType(
      m_ProbabilityImageVector[c],
      m_ProbabilityImageVector[c]->GetLargestPossibleRegion() );
    probIt[c]->GoToBegin();
    }

  typedef itk::ImageRegionConstIteratorWithIndex< LabelMapType >
    ConstLabelMapIteratorType;
  ConstLabelMapIteratorType itInLabelMap( m_LabelMap,
    m_LabelMap->GetLargestPossibleRegion() );
  itInLabelMap.GoToBegin();

  typename LabelMapType::IndexType indx;
  FeatureVectorType fv;
  while( !itInLabelMap.IsAtEnd() )
    {
    indx = itInLabelMap.GetIndex();
    fv = m_FeatureVectorGenerator->GetFeatureVector( indx );

    for( unsigned int c=0; c<numClasses; ++c )
      {
      double prob = this->GetClassProbability( c, fv );
      prob *= m_PDFWeightList[c];
      probIt[c]->Set( prob );

      ++( *( probIt[c] ) );
      }

    ++itInLabelMap;
    }

  for( unsigned int c = 0; c < numClasses; ++c )
    {
    delete probIt[c];
    }

  typedef itk::tube::SmoothingRecursiveGaussianImageFilter<
    ProbabilityImageType, ProbabilityImageType > ProbImageFilterType;
  typename ProbImageFilterType::Pointer probImageFilter;

  typedef itk::ThresholdImageFilter< ProbabilityImageType >
    ThresholdProbImageFilterType;
  typename ThresholdProbImageFilterType::Pointer
    thresholdProbImageFilter = ThresholdProbImageFilterType::New();

  for( unsigned int c = 0; c < numClasses; c++ )
    {
    probImageFilter = ProbImageFilterType::New();
    probImageFilter->SetInput( m_ProbabilityImageVector[c] );
    probImageFilter->SetSigma(
      m_ProbabilityImageSmoothingStandardDeviation );
    probImageFilter->Update();
    m_ProbabilityImageVector[c] = probImageFilter->GetOutput();

    thresholdProbImageFilter = ThresholdProbImageFilterType::New();
    thresholdProbImageFilter->SetInput( m_ProbabilityImageVector[c] );
    thresholdProbImageFilter->SetOutsideValue( 0 );
    thresholdProbImageFilter->ThresholdBelow( 0 );
    thresholdProbImageFilter->Update();
    m_ProbabilityImageVector[c] = thresholdProbImageFilter->GetOutput();
    }

  //
  //  Create label image
  //

  typename LabelMapType::IndexType labelImageIndex;

  typename LabelMapType::Pointer tmpLabelImage = LabelMapType::New();
  tmpLabelImage->SetRegions( m_FeatureVectorGenerator->GetInput( 0 )
    ->GetLargestPossibleRegion() );
  tmpLabelImage->CopyInformation( m_FeatureVectorGenerator->GetInput( 0 ) );
  tmpLabelImage->Allocate();

  if( m_LabelMap.IsNull() )
    {
    m_LabelMap = tmpLabelImage;
    m_LabelMap->FillBuffer( m_VoidId );
    m_ForceClassification = true;
    }

  if( !m_ForceClassification )
    {
    for( unsigned int c = 0; c < numClasses; c++ )
      {
      // For this class, label all pixels for which it is the most
      // likely class.
      itk::ImageRegionIteratorWithIndex<LabelMapType> labelIt(
        tmpLabelImage, tmpLabelImage->GetLargestPossibleRegion() );
      labelIt.GoToBegin();
      while( !labelIt.IsAtEnd() )
        {
        labelImageIndex = labelIt.GetIndex();
        bool maxPC = true;
        double maxP = m_ProbabilityImageVector[c]->GetPixel(
          labelImageIndex );
        for( unsigned int oc = 0; oc < numClasses; oc++ )
          {
          if( oc != c &&
              m_ProbabilityImageVector[oc]->GetPixel( labelImageIndex )
              > maxP )
            {
            maxPC = false;
            break;
            }
          }
        if( maxPC )
          {
          labelIt.Set( 128 );
          }
        else
          {
          labelIt.Set( 0 );
          }
        ++labelIt;
        }

      typedef itk::ConnectedThresholdImageFilter<LabelMapType,
        LabelMapType> ConnectedFilterType;
      typename ConnectedFilterType::Pointer insideConnecter =
        ConnectedFilterType::New();

      typename ListSampleType::ConstIterator
        inClassListIt( m_InClassList[c]->Begin() );
      typename ListSampleType::ConstIterator
        inClassListItEnd( m_InClassList[c]->End() );
      typename ImageType::IndexType indx;
      while( inClassListIt != inClassListItEnd )
        {
        for( unsigned int i = 0; i < ImageDimension; i++ )
          {
          indx[i] = static_cast<int>(
            inClassListIt.GetMeasurementVector()[N+i] );
          }
        insideConnecter->AddSeed( indx );
        ++inClassListIt;
        }

      if( !m_ReclassifyObjectLabels )
        {
        // The pixels with maximum probability for the current
        // class are all set to 128 before the update of the
        // ConnectedThresholdImageFilter, so if the input label
        // map is not to be reclassified, set the pixels belonging
        // to this class to 128 before updating the filter regardless
        // of the probability.  Setting input labels to 255 before
        // the update
        // of the ConnectedThresholdFilter will cause the filter to
        // return only the values at 255 (the input label map).
        inClassListIt = m_InClassList[c]->Begin();
        inClassListItEnd = m_InClassList[c]->End();
        while( inClassListIt != inClassListItEnd )
          {
          for( unsigned int i = 0; i < ImageDimension; i++ )
            {
            indx[i] = static_cast<int>(
              inClassListIt.GetMeasurementVector()[N+i] );
            }
          tmpLabelImage->SetPixel( indx, 128 );
          ++inClassListIt;
          }
        for( unsigned int oc = 0; oc < numClasses; oc++ )
          {
          if( oc != c )
            {
            // Use inside mask to set seed points.  Also draw inside mask
            // in
            // label image to ensure those points are considered object
            // points.
            // Erase other mask from label image
            inClassListIt = m_InClassList[oc]->Begin();
            inClassListItEnd = m_InClassList[oc]->End();
            while( inClassListIt != inClassListItEnd )
              {
              for( unsigned int i = 0; i < ImageDimension; i++ )
                {
                indx[i] = static_cast<int>(
                  inClassListIt.GetMeasurementVector()[N+i] );
                }
              tmpLabelImage->SetPixel( indx, 0 );
              ++inClassListIt;
              }
            }
          }

        // Erase outside mask from label image
        typename ListSampleType::ConstIterator
          outListIt( m_OutClassList->Begin() );
        typename ListSampleType::ConstIterator
          outListItEnd( m_OutClassList->End() );
        while( outListIt != outListItEnd )
          {
          for( unsigned int i = 0; i < ImageDimension; i++ )
            {
            indx[i] = static_cast<int>(
              outListIt.GetMeasurementVector()[N+i] );
            }
          tmpLabelImage->SetPixel( indx, 0 );
          ++outListIt;
          }
        }

      insideConnecter->SetInput( tmpLabelImage );
      insideConnecter->SetLower( 64 );
      insideConnecter->SetUpper( 194 );
      insideConnecter->SetReplaceValue( 255 );
      insideConnecter->Update();
      tmpLabelImage = insideConnecter->GetOutput();

      //
      // Fill holes
      //
      if( holeFillIterations > 0 )
        {
        typedef itk::VotingBinaryIterativeHoleFillingImageFilter<
          LabelMapType > HoleFillingFilterType;

        typename HoleFillingFilterType::Pointer holeFiller =
          HoleFillingFilterType::New();
        typename LabelMapType::SizeType holeRadius;
        holeRadius.Fill( 1 );
        holeFiller->SetInput( tmpLabelImage );
        holeFiller->SetRadius( holeRadius );
        holeFiller->SetBackgroundValue( 0 );
        holeFiller->SetForegroundValue( 255 );
        holeFiller->SetMajorityThreshold( 2 );
        holeFiller->SetMaximumNumberOfIterations( holeFillIterations );
        holeFiller->Update();
        tmpLabelImage = holeFiller->GetOutput();
        }

      //
      // Erode
      //
      typedef BinaryBallStructuringElement< LabelMapPixelType,
        ImageType::ImageDimension >                  StructuringElementType;
      typedef BinaryErodeImageFilter< LabelMapType, LabelMapType,
        StructuringElementType >                     ErodeFilterType;
      typedef itk::BinaryDilateImageFilter< LabelMapType,
        LabelMapType, StructuringElementType >       DilateFilterType;

      StructuringElementType sphereOp;
      if( erodeRadius > 0 )
        {
        sphereOp.SetRadius( erodeRadius );
        sphereOp.CreateStructuringElement();

        if( m_DilateFirst )
          {
          typename DilateFilterType::Pointer insideLabelMapDilateFilter =
            DilateFilterType::New();
          insideLabelMapDilateFilter->SetKernel( sphereOp );
          insideLabelMapDilateFilter->SetDilateValue( 255 );
          insideLabelMapDilateFilter->SetInput( tmpLabelImage );
          insideLabelMapDilateFilter->Update();
          tmpLabelImage = insideLabelMapDilateFilter->GetOutput();
          }
        else
          {
          typename ErodeFilterType::Pointer insideLabelMapErodeFilter =
            ErodeFilterType::New();
          insideLabelMapErodeFilter->SetKernel( sphereOp );
          insideLabelMapErodeFilter->SetErodeValue( 255 );
          insideLabelMapErodeFilter->SetInput( tmpLabelImage );
          insideLabelMapErodeFilter->Update();
          tmpLabelImage = insideLabelMapErodeFilter->GetOutput();
          }
        }

      //
      // Re-do connectivity
      //
      if( true ) // creating a local context to limit memory footprint
        {
        typedef itk::ConnectedThresholdImageFilter<LabelMapType,
          LabelMapType> ConnectedLabelMapFilterType;

        typename ConnectedLabelMapFilterType::Pointer
          insideConnectedLabelMapFilter = ConnectedLabelMapFilterType::New();
        insideConnectedLabelMapFilter->SetInput( tmpLabelImage );
        insideConnectedLabelMapFilter->SetLower( 194 );
        insideConnectedLabelMapFilter->SetUpper( 255 );
        insideConnectedLabelMapFilter->SetReplaceValue( 255 );

        // Use inside mask to set seed points.  Also draw inside mask in
        // label image to ensure those points are considered object points
        inClassListIt = m_InClassList[c]->Begin();
        inClassListItEnd = m_InClassList[c]->End();
        while( inClassListIt != inClassListItEnd )
          {
          for( unsigned int i = 0; i < ImageDimension; i++ )
            {
            indx[i] = static_cast<int>(
              inClassListIt.GetMeasurementVector()[N+i] );
            }

          insideConnectedLabelMapFilter->AddSeed( indx );
          // Don't redraw objects
          ++inClassListIt;
          }

        insideConnectedLabelMapFilter->Update();
        tmpLabelImage = insideConnectedLabelMapFilter->GetOutput();
        }

      //
      // Dilate back to original size
      //
      if( erodeRadius > 0 )
        {
        if( m_DilateFirst )
          {
          typename ErodeFilterType::Pointer insideLabelMapErodeFilter =
            ErodeFilterType::New();
          insideLabelMapErodeFilter->SetKernel( sphereOp );
          insideLabelMapErodeFilter->SetErodeValue( 255 );
          insideLabelMapErodeFilter->SetInput( tmpLabelImage );
          insideLabelMapErodeFilter->Update();
          tmpLabelImage = insideLabelMapErodeFilter->GetOutput();
          }
        else
          {
          typename DilateFilterType::Pointer insideLabelMapDilateFilter =
            DilateFilterType::New();
          insideLabelMapDilateFilter->SetKernel( sphereOp );
          insideLabelMapDilateFilter->SetDilateValue( 255 );
          insideLabelMapDilateFilter->SetInput( tmpLabelImage );
          insideLabelMapDilateFilter->Update();
          tmpLabelImage = insideLabelMapDilateFilter->GetOutput();
          }
        }

      // Merge with input mask
      typedef itk::ImageRegionIterator< LabelMapType >
        LabelMapIteratorType;
      LabelMapIteratorType itInLabelMap( m_LabelMap,
        m_LabelMap->GetLargestPossibleRegion() );
      itInLabelMap.GoToBegin();

      LabelMapIteratorType itLabel( tmpLabelImage,
        tmpLabelImage->GetLargestPossibleRegion() );
      itLabel.GoToBegin();

      while( !itInLabelMap.IsAtEnd() )
        {
        if( itLabel.Get() == 255 )
          {
          if( itInLabelMap.Get() == m_VoidId
            || ( m_ReclassifyObjectLabels && m_ReclassifyNotObjectLabels ) )
            {
            itInLabelMap.Set( m_ObjectIdList[c] );
            }
          else
            {
            if( m_ReclassifyObjectLabels || m_ReclassifyNotObjectLabels )
              {
              bool isObjectId = false;
              for( unsigned int oc = 0; oc < numClasses; oc++ )
                {
                if( itInLabelMap.Get() == m_ObjectIdList[oc] )
                  {
                  isObjectId = true;
                  break;
                  }
                }
              if( ( isObjectId && m_ReclassifyObjectLabels ) ||
                  ( !isObjectId && m_ReclassifyNotObjectLabels ) )
                {
                itInLabelMap.Set( m_ObjectIdList[c] );
                }
              }
            }
          }
        else
          {
          if( itInLabelMap.Get() == m_ObjectIdList[c] )
            {
            if( m_ReclassifyObjectLabels )
              {
              itInLabelMap.Set( m_VoidId );
              }
            }
          }
        ++itInLabelMap;
        ++itLabel;
        }
      }
    }
  else
    {
    // Merge with input mask
    typedef itk::ImageRegionIteratorWithIndex< LabelMapType >
      LabelMapIteratorType;
    LabelMapIteratorType itInLabelMap( m_LabelMap,
      m_LabelMap->GetLargestPossibleRegion() );
    itInLabelMap.GoToBegin();

    while( !itInLabelMap.IsAtEnd() )
      {
      labelImageIndex = itInLabelMap.GetIndex();
      unsigned int maxPC = 0;
      double maxP = m_ProbabilityImageVector[0]->GetPixel(
        labelImageIndex );
      for( unsigned int c = 1; c < numClasses; c++ )
        {
        double p = m_ProbabilityImageVector[c]->GetPixel(
          labelImageIndex );
        if( p > maxP )
          {
          maxP = p;
          maxPC = c;
          }
        }
      if( itInLabelMap.Get() == m_VoidId
        || ( m_ReclassifyObjectLabels && m_ReclassifyNotObjectLabels ) )
        {
        itInLabelMap.Set( m_ObjectIdList[maxPC] );
        }
      else
        {
        if( m_ReclassifyObjectLabels || m_ReclassifyNotObjectLabels )
          {
          bool isObjectId = false;
          for( unsigned int oc = 0; oc < numClasses; oc++ )
            {
            if( itInLabelMap.Get() == m_ObjectIdList[oc] )
              {
              isObjectId = true;
              break;
              }
            }
          if( ( isObjectId && m_ReclassifyObjectLabels ) ||
              ( !isObjectId && m_ReclassifyNotObjectLabels ) )
            {
            itInLabelMap.Set( m_ObjectIdList[maxPC] );
            }
          }
        }
      ++itInLabelMap;
      }
    }

  m_ClassProbabilityImagesUpToDate = true;
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::Update( void )
{
  this->GenerateSample();
  this->GeneratePDFs();
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::ClassifyImages( void )
{
  this->ApplyPDFs();
}

template< class TImage, unsigned int N, class TLabelMap >
void
PDFSegmenterBase< TImage, N, TLabelMap >
::PrintSelf( std::ostream & os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );

  if( m_SampleUpToDate )
    {
    os << indent << "SampleUpToDate = true" << std::endl;
    }
  else
    {
    os << indent << "SampleUpToDate = false" << std::endl;
    }

  if( m_PDFsUpToDate )
    {
    os << indent << "PDFsUpToDate = true" << std::endl;
    }
  else
    {
    os << indent << "PDFsUpToDate = false" << std::endl;
    }

  if( m_ClassProbabilityImagesUpToDate )
    {
    os << indent << "ClassProbabilityImagesUpToDate = true" << std::endl;
    }
  else
    {
    os << indent << "ClassProbabilityImagesUpToDate = false" << std::endl;
    }

  os << indent << "Feature vector generator = " << m_FeatureVectorGenerator
    << std::endl;

  if( m_LabelMap.IsNotNull() )
    {
    os << indent << "LabelMap = " << m_LabelMap << std::endl;
    }
  else
    {
    os << indent << "LabelMap = NULL" << std::endl;
    }
  os << indent << "Erode radius = " << m_ErodeRadius << std::endl;
  os << indent << "Hole fill iterations = " << m_HoleFillIterations
    << std::endl;
  os << indent << "PDF weight size = " << m_PDFWeightList.size()
    << std::endl;
  os << indent << "Probability Smoothing Standard Deviation = "
    << m_ProbabilityImageSmoothingStandardDeviation << std::endl;
  os << indent << "ReclassifyObjectLabels = " << m_ReclassifyObjectLabels
    << std::endl;
  os << indent << "ReclassifyNotObjectLabels = "
    << m_ReclassifyNotObjectLabels << std::endl;
  os << indent << "Number of probability images = "
    << m_ProbabilityImageVector.size() << std::endl;
  os << indent << "InClassList size = "
    << m_InClassList.size() << std::endl;
  if( m_OutClassList.IsNotNull() )
    {
    os << indent << "OutClassList = " << m_OutClassList << std::endl;
    }
  else
    {
    os << indent << "OutClassList = NULL" << std::endl;
    }
}

} // End namespace tube

} // End namespace itk

#endif // End !defined(__itktubePDFSegmenterBase_hxx)
