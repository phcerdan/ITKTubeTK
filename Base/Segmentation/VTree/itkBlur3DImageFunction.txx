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
#ifndef _itkBlur3DImageFunction_txx
#define _itkBlur3DImageFunction_txx

#include "itkBlur3DImageFunction.h"

#include <cmath>
#include <algorithm>

#include "itkImage.h"
#include "itkContinuousIndex.h"


namespace itk
{
/**
 * Set the input Image */
template <class TInputImage>
Blur3DImageFunction<TInputImage>
::Blur3DImageFunction( )
{
  m_Debug = false;

  this->m_Image = NULL;
  m_Spacing.Fill(0);
  m_OriginalSpacing.Fill(0);
  m_UseRelativeSpacing = true;
  m_Scale = 1;
  m_Extent = 3.1;
  m_ImageIndexMin.Fill(0);
  m_ImageIndexMax.Fill(0);
}

/**
 * Set the input Image */
template <class TInputImage>
void
Blur3DImageFunction<TInputImage>
::SetInputImage( const InputImageType * ptr )
{
  this->Superclass::SetInputImage( ptr );
  m_Spacing  = this->GetInputImage()->GetSpacing();
  m_OriginalSpacing  = this->GetInputImage()->GetSpacing();
  if(m_UseRelativeSpacing)
    {
    for(int i=0; i<ImageDimension; i++)
      {
      m_Spacing[i] = m_OriginalSpacing[i] / m_OriginalSpacing[0];
      }
    }

  m_ImageIndexMin = 
     this->GetInputImage()->GetLargestPossibleRegion().GetIndex();
  for( unsigned int i=0; i<ImageDimension; i++ )
    {
    m_ImageIndexMax[i] = m_ImageIndexMin[i]
      + this->GetInputImage()->GetLargestPossibleRegion().GetSize()[i]
      - 1;
    }

  /* Values by default */
  this->RecomputeKernel();

}


/**
 * Print */
template<class TInputImage>
void
Blur3DImageFunction<TInputImage>
::SetUseRelativeSpacing(bool useRelativeSpacing) 
{
  m_UseRelativeSpacing = useRelativeSpacing;
  if(m_UseRelativeSpacing)
    {
    for(int i=0; i<ImageDimension; i++)
      {
      m_Spacing[i] = m_OriginalSpacing[i] / m_OriginalSpacing[0];
      }
    }
  else
    {
    for(int i=0; i<ImageDimension; i++)
      {
      m_Spacing[i] = m_OriginalSpacing[i];
      }
    }

}

/**
 * Print */
template<class TInputImage>
void
Blur3DImageFunction<TInputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "calculate Blurring value at point:" << std::endl;
  std::cout << "Debug = " << m_Debug << std::endl;
  std::cout << "UseRelativeSpacing = " << m_UseRelativeSpacing << std::endl;
  std::cout << "Spacing = " << m_Spacing << std::endl;
  std::cout << "OriginalSpacing = " << m_OriginalSpacing << std::endl;
  std::cout << "Scale = " << m_Scale << std::endl;
  std::cout << "Extent = " << m_Extent << std::endl;
  std::cout << "KernelWeights.size = " << m_KernelWeights.size() << std::endl;
  std::cout << "KernelX.size = " << m_KernelX.size() << std::endl;
  std::cout << "KernelMin = " << m_KernelMin << std::endl;
  std::cout << "KernelMax = " << m_KernelMax << std::endl;
  std::cout << "KernelTotal = " << m_KernelTotal << std::endl;

  std::cout << "ImageIndexMin = " << m_ImageIndexMin << std::endl;
  std::cout << "ImageIndexMax = " << m_ImageIndexMax << std::endl;
}


/**
 * SetScale
 * Pre-compute kernel weights */
template<class TInputImage>
void
Blur3DImageFunction<TInputImage>
::RecomputeKernel( void )
{
  if(m_Debug)
    {
    std::cout << "RecomputeKernel" << std::endl;
    }
  double gfact = -0.5/(m_Scale*m_Scale);

  for(int i=0; i<ImageDimension; i++)
    {
    m_KernelMax[i] = (int)((m_Scale*m_Extent)/m_Spacing[i]);
    if(m_KernelMax[i]<1)
      {
      m_KernelMax[i] = 1;
      }
    m_KernelMin[i] = -m_KernelMax[i];
    }
  if(m_Debug)
    {
    std::cout << "  Scale = " << m_Scale << std::endl;
    std::cout << "  Extent = " << m_Extent << std::endl;
    std::cout << "  KernelMin = " << m_KernelMin << std::endl;
    std::cout << "  KernelMax = " << m_KernelMax << std::endl;
    }
  
  m_KernelWeights.clear();
  m_KernelX.clear();

  IndexType index;
  m_KernelTotal = 0;
  if( ImageDimension == 3 )
    {
    for(index[2] = m_KernelMin[2]; index[2]<=m_KernelMax[2]; index[2]++)
      {
      double distZ = index[2] * m_Spacing[2];
      distZ = distZ * distZ;
      for(index[1] = m_KernelMin[1]; index[1]<=m_KernelMax[1]; index[1]++)
        {
        double distY = index[1] * m_Spacing[1];
        distY = distY * distY + distZ;
        for(index[0] = m_KernelMin[0]; index[0]<=m_KernelMax[0]; index[0]++)
          {
          double dist = index[0] * m_Spacing[0];
          dist = dist * dist + distY;
          double w = exp(gfact*(dist));
          m_KernelWeights.push_back(w);
          m_KernelX.push_back(index);
          m_KernelTotal += w;
          }
        }
      }
    }
  else if( ImageDimension == 2 )
    {
    for(index[1] = m_KernelMin[1]; index[1]<=m_KernelMax[1]; index[1]++)
      {
      double distY = index[1] * m_Spacing[1];
      distY = distY * distY;
      for(index[0] = m_KernelMin[0]; index[0]<=m_KernelMax[0]; index[0]++)
        {
        double dist = index[0] * m_Spacing[0];
        dist = dist * dist + distY;
        double w = exp(gfact*(dist));
        m_KernelWeights.push_back(w);
        m_KernelX.push_back(index);
        m_KernelTotal += w;
        }
      }
    }
}

/**
 * SetScale
 * Pre-compute kernel weights */
template<class TInputImage>
void
Blur3DImageFunction<TInputImage>
::SetScale(double scale)
{
  if(m_Scale != scale)
    {
    m_Scale = scale;
    this->RecomputeKernel();
    }
}

/**
 * SetExtent
 * Pre-compute kernel weights */
template<class TInputImage>
void
Blur3DImageFunction<TInputImage>
::SetExtent(double extent)
{
  if(m_Extent != extent)
    {
    m_Extent = extent;
    this->RecomputeKernel();
    }
}

/**
 * Evaluate the fonction at the specified point */
template <class TInputImage>
double
Blur3DImageFunction<TInputImage>
::Evaluate(const PointType& point) const
{
  if(m_Debug)
    {
    std::cout << "Blur3DImageFunction::Evaluate" << std::endl;
    }

  ContinuousIndexType index;
  if( !this->m_Image )
    {
    for( unsigned int i=0; i<ImageDimension; i++ )
      {
      index[i] = point[i];
      }
    }
  else
    {
    this->m_Image->TransformPhysicalPointToContinuousIndex(point,index);
    }

  if(m_Debug)
    {
    std::cout << "  Calling EvaluateAtContinuousIndex " << std::endl;
    }
  return this->EvaluateAtContinuousIndex(index);
}

template <class TInputImage>
double
Blur3DImageFunction<TInputImage>
::EvaluateAtIndex(const IndexType & point) const
{
  if(m_Debug)
    {
    std::cout << "Blur3DImageFunction::EvaluateAtIndex" << std::endl;
    std::cout << "  Point = " << point << std::endl;
    }

  if( !this->m_Image )
    {
    return 0.0;
    }
  
  double res = 0;
  double wTotal;

  IndexType kernelX;

  bool boundary = false;
  for(unsigned int i=0; i<ImageDimension; i++)
    {
    if(point[i]+m_KernelMin[i]<m_ImageIndexMin[i] 
       || point[i]+m_KernelMax[i]>m_ImageIndexMax[i])
      {
      boundary = true;
      break;
      }
    }

  if(!boundary)
    {
    KernelWeightsListType::const_iterator it;
    KernelWeightsListType::const_iterator itEnd;
    typename KernelXListType::const_iterator  itX;
    it = m_KernelWeights.begin();
    itEnd = m_KernelWeights.end();
    itX = m_KernelX.begin();
    while(it != itEnd)
      {
      for( unsigned int i=0; i<ImageDimension; i++ )
        {
        kernelX[i] = point[i] + (*itX)[i];
        }
      res +=  this->m_Image->GetPixel( kernelX ) * (*it);
      ++it;
      ++itX;
      }
    wTotal = m_KernelTotal; 
    }
  else
    {
    if(m_Debug)
      {
      std::cout << "  Boundary point" << std::endl;
      }
    KernelWeightsListType::const_iterator it;
    KernelWeightsListType::const_iterator itEnd;
    typename KernelXListType::const_iterator  itX;
    it = m_KernelWeights.begin();
    itEnd = m_KernelWeights.end();
    itX = m_KernelX.begin();
    wTotal = 0; 
    double w;
    while(it != itEnd)
      {
      bool valid = true;
      for( unsigned int i=0; i<ImageDimension; i++ )
        {
        kernelX[i] = point[i] + (*itX)[i];
        if( kernelX[i] < m_ImageIndexMin[i] ||
            kernelX[i] > m_ImageIndexMax[i] )
          {
          valid = false;
          break;
          }
        }
      if( valid )
        {
        w = *it;
        res +=  this->m_Image->GetPixel( kernelX ) * w;
        wTotal += w;
        }
      ++it;
      ++itX;
      }
    }
  
  if(wTotal < *(m_KernelWeights.begin()) )
    { 
    return 0;
    }

  if(m_Debug)
    {
    std::cout << "  result = " << res/wTotal << std::endl;
    }
  return res/wTotal;
}

template <class TInputImage>
double
Blur3DImageFunction<TInputImage>
::EvaluateAtContinuousIndex(const ContinuousIndexType & point) const
{
  if(m_Debug)
    {
    std::cout << "Blur3DImageFunction::EvaluateAtContinuousIndex" 
      << std::endl;
    std::cout << "  Point = " << point << std::endl;
    }

  if( !this->m_Image )
    {
    return 0.0;
    }
  
  double w;
  double res = 0;
  double wTotal = 0;
  double gfact = -0.5/(m_Scale*m_Scale);
  double kernrad = m_Scale*m_Extent*m_Scale*m_Extent;

  IndexType kernelX;

  bool boundary = false;
  for(unsigned int i=0; i<ImageDimension; i++)
    {
    if((int)(point[i])+m_KernelMin[i]<m_ImageIndexMin[i] 
      || (int)(point[i])+m_KernelMax[i]>m_ImageIndexMax[i])
      {
      boundary = true;
      break;
      }
    }

  if(!boundary)
    {
    if( ImageDimension == 3 )
      {
      for(int z = m_KernelMin[2]; z<=m_KernelMax[2]; z++)
        {
        kernelX[2] = (int)(point[2])+z;
        double distZ = (kernelX[2]-point[2])*m_Spacing[2];
        distZ = distZ * distZ;
        for(int y = m_KernelMin[1]; y<=m_KernelMax[1]; y++)
          {
          kernelX[1] = (int)(point[1])+y;
          double distY = (kernelX[1]-point[1])*m_Spacing[1];
          distY = distY * distY + distZ;
          for(int x = m_KernelMin[0]; x<=m_KernelMax[0]; x++)
            {
            kernelX[0] = (int)(point[0])+x;
            double distX = (kernelX[0]-point[0])*m_Spacing[0];
            double dist = distX * distX + distY;
            if( dist <= kernrad )
              {
              w = exp(gfact*dist);
              wTotal += w; 
              res +=  this->m_Image->GetPixel( kernelX ) * w ;     
              }
            }
          }
        }
      }
    else if( ImageDimension == 2 )
      {
      for(int y = m_KernelMin[1]; y<=m_KernelMax[1]; y++)
        {
        kernelX[1] = (int)(point[1])+y;
        double distY = (kernelX[1]-point[1])*m_Spacing[1];
        distY = distY * distY;
        for(int x = m_KernelMin[0]; x<=m_KernelMax[0]; x++)
          {
          kernelX[0] = (int)(point[0])+x;
          double distX = (kernelX[0]-point[0])*m_Spacing[0];
          double dist = distX * distX + distY;
          if( dist <= kernrad )
            {
            w = exp(gfact*dist);
            wTotal += w; 
            res +=  this->m_Image->GetPixel( kernelX ) * w ;     
            }
          }
        }
      }
    }
  else
    {
    if(m_Debug)
      {
      std::cout << "  Boundary point" << std::endl;
      }
    IndexType minX;
    IndexType maxX;
    for(unsigned int i=0; i<ImageDimension; i++)
      {
      minX[i] = vnl_math_max((int)((int)(point[i])+m_KernelMin[i]), 
        (int)(m_ImageIndexMin[i]));
      maxX[i] = vnl_math_min((int)((int)(point[i])+m_KernelMax[i]),
        (int)(m_ImageIndexMax[i]));
      }

    if( ImageDimension == 3 )
      {
      for(kernelX[2] = minX[2]; kernelX[2]<=maxX[2]; kernelX[2]++)
        {
        double distZ = (kernelX[2]-point[2])*m_Spacing[2];
        distZ = distZ * distZ;
        for(kernelX[1] = minX[1]; kernelX[1]<=maxX[1]; kernelX[1]++)
          {
          double distY = (kernelX[1]-point[1])*m_Spacing[1];
          distY = distY * distY + distZ;
          for(kernelX[0] = minX[0]; kernelX[0]<=maxX[0]; kernelX[0]++)
            {
            double distX = (kernelX[0]-point[0])*m_Spacing[0];
            double dist = distX * distX + distY;
            if( dist <= kernrad )
              {
              w = exp(gfact*(dist));
              wTotal += w; 
              res +=  this->m_Image->GetPixel( kernelX ) * w ;     
              }
            }
          }
        }
      }
    else if( ImageDimension == 2 )
      {
      for(kernelX[1] = minX[1]; kernelX[1]<=maxX[1]; kernelX[1]++)
        {
        double distY = (kernelX[1]-point[1])*m_Spacing[1];
        distY = distY * distY;
        for(kernelX[0] = minX[0]; kernelX[0]<=maxX[0]; kernelX[0]++)
          {
          double distX = (kernelX[0]-point[0])*m_Spacing[0];
          double dist = distX * distX + distY;
          if( dist <= kernrad )
            {
            w = exp(gfact*(dist));
            wTotal += w; 
            res +=  this->m_Image->GetPixel( kernelX ) * w ;     
            }
          }
        }
      }
    }
  
  if(wTotal < *(m_KernelWeights.begin()) )
    { 
    return 0;
    }
  if(m_Debug)
    {
    std::cout << "  result = " << res/wTotal << std::endl;
    }
  return res/wTotal;
}

} // namespace itk

#endif
