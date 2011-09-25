/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END NIGHTINGALE GPL
//
*/

#ifndef __SBRATINGPROPERTYBUILDER_H__
#define __SBRATINGPROPERTYBUILDER_H__

#include "sbAbstractPropertyBuilder.h"

#include <sbIPropertyBuilder.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbRatingPropertyBuilder : public sbAbstractPropertyBuilder,
                                public sbIRatingPropertyBuilder
{
public:

  virtual ~sbRatingPropertyBuilder() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_SBIPROPERTYBUILDER_NO_GET(sbAbstractPropertyBuilder::)
  NS_DECL_SBIRATINGPROPERTYBUILDER

  NS_IMETHOD Get(sbIPropertyInfo **_retval);
};

#endif /* __SBRATINGPROPERTYBUILDER_H__ */

