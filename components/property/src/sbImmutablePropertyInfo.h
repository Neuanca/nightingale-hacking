/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
*/

#ifndef __SBIMMUTABLEPROPERTYINFO_H__
#define __SBIMMUTABLEPROPERTYINFO_H__

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>

#include <nsIStringBundle.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsCOMArray.h>
#include "sbPropertyUnitConverter.h"

class sbImmutablePropertyInfo : public sbIPropertyInfo
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYINFO

  sbImmutablePropertyInfo();
  virtual ~sbImmutablePropertyInfo();

  NS_IMETHOD SetUnitConverter(sbIPropertyUnitConverter *aUnitConverter);

protected:
  nsresult Init();

  PRUint32 mNullSort;
  nsCOMPtr<sbIPropertyArray> mSortProfile;
  nsString mID;
  nsString mType;
  nsString mDisplayName;
  PRBool mUserViewable;
  PRBool mUserEditable;
  PRBool mRemoteReadable;
  PRBool mRemoteWritable;
  nsCOMPtr<nsIStringBundle> mBundle;
  PRLock*   mOperatorsLock;
  nsCOMArray<sbIPropertyOperator> mOperators;

  nsCOMPtr<sbIPropertyUnitConverter> mUnitConverter;
};

#endif /* __SBIMMUTABLEPROPERTYINFO_H__ */

