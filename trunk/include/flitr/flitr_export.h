/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
 * 
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef FLITR_EXPORT_H
#define FLITR_EXPORT_H 1

#ifdef _MSC_VER
#  if defined( FLITR_STATIC_CPP )
#    define FLITR_EXPORT
#  elif defined( FLITR_SHARED_LIBRARY )
#    define FLITR_EXPORT __declspec(dllexport)
#  else
#    define FLITR_EXPORT __declspec(dllimport)
#  endif
#else
#  define FLITR_EXPORT
#endif

#endif // FLITR_EXPORT_H
