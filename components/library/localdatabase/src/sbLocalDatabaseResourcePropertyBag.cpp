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


#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabasePropertyCache.h"

#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsStringEnumerator.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>
#include <nsXPCOMCIDInternal.h>
#include <prlog.h>

#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include "sbLocalDatabaseSQL.h"
#include <sbTArrayStringEnumerator.h>
#include <sbStringUtils.h>

#ifdef PR_LOGGING
extern PRLogModuleInfo *gLocalDatabasePropertyCacheLog;
#endif

#define TRACE(args) PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_WARN, args)

PRUint32 const BAG_HASHTABLE_SIZE = 20;

// sbILocalDatabaseResourcePropertyBag
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseResourcePropertyBag,
                              sbILocalDatabaseResourcePropertyBag)

sbLocalDatabaseResourcePropertyBag::sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                                                       PRUint32 aMediaItemId,
                                                                       const nsAString &aGuid)
: mCache(aCache)
, mGuid(aGuid)
, mMediaItemId(aMediaItemId)
, mDirtyLock(nsnull)
{
}

sbLocalDatabaseResourcePropertyBag::~sbLocalDatabaseResourcePropertyBag()
{
  if(mDirtyLock) {
    nsAutoLock::DestroyLock(mDirtyLock);
  }
}

nsresult
sbLocalDatabaseResourcePropertyBag::Init()
{
  nsresult rv;

  PRBool success = mValueMap.Init(BAG_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDirty.Init(BAG_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mPropertyManager = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mDirtyLock = nsAutoLock::NewLock("sbLocalDatabaseResourcePropertyBag::mDirtyLock");
  NS_ENSURE_TRUE(mDirtyLock, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseResourcePropertyBag::PropertyBagKeysToArray(const PRUint32& aPropertyID,
                                                           sbPropertyData* aPropertyData,
                                                           void *aArg)
{
  nsTArray<PRUint32>* propertyIDs = static_cast<nsTArray<PRUint32>*>(aArg);
  if (propertyIDs->AppendElement(aPropertyID)) {
    return PL_DHASH_NEXT;
  }
  else {
    return PL_DHASH_STOP;
  }
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetGuid(nsAString &aGuid)
{
  aGuid = mGuid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetMediaItemId(PRUint32* aMediaItemId)
{
  NS_ENSURE_ARG_POINTER(aMediaItemId);
  *aMediaItemId = mMediaItemId;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetIds(nsIStringEnumerator **aIDs)
{
  NS_ENSURE_ARG_POINTER(aIDs);

  nsTArray<PRUint32> propertyDBIDs;
  mValueMap.EnumerateRead(PropertyBagKeysToArray, &propertyDBIDs);

  PRUint32 len = mValueMap.Count();
  if (propertyDBIDs.Length() < len) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTArray<nsString> propertyIDs;
  for (PRUint32 i = 0; i < len; i++) {
    nsString propertyID;
    PRBool success = mCache->GetPropertyID(propertyDBIDs[i], propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);
    propertyIDs.AppendElement(propertyID);
  }

  *aIDs = new sbTArrayStringEnumerator(&propertyIDs);
  NS_ENSURE_TRUE(*aIDs, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aIDs);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetProperty(const nsAString& aPropertyID,
                                                nsAString& _retval)
{
  PRUint32 propertyDBID = mCache->GetPropertyDBIDInternal(aPropertyID);
  return GetPropertyByID(propertyDBID, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetPropertyByID(PRUint32 aPropertyDBID,
                                                    nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoLock lock(mDirtyLock);
    sbPropertyData* data;

    if (mValueMap.Get(aPropertyDBID, &data)) {
      return data->GetValue(_retval);
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetSortablePropertyByID(PRUint32 aPropertyDBID,
                                                            nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoLock lock(mDirtyLock);
    sbPropertyData* data;

    if (mValueMap.Get(aPropertyDBID, &data)) {
      return data->GetSortableValue(_retval);
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::
  GetSearchablePropertyByID(PRUint32 aPropertyDBID,
                            nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoLock lock(mDirtyLock);
    sbPropertyData* data;

    if (mValueMap.Get(aPropertyDBID, &data)) {
      return data->GetSearchableValue(_retval);
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::SetProperty(const nsAString & aPropertyID,
                                                const nsAString & aValue)
{
  nsresult rv;

  PRUint32 propertyDBID = mCache->GetPropertyDBIDInternal(aPropertyID);
  if(propertyDBID == 0) {
    rv = mCache->InsertPropertyIDInLibrary(aPropertyID, &propertyDBID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = mPropertyManager->GetPropertyInfo(aPropertyID,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool valid = PR_FALSE;
  rv = propertyInfo->Validate(aValue, &valid);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(PR_LOGGING)
  if(NS_UNLIKELY(!valid)) {
    LOG(("Failed to set property id %s with value %s",
         NS_ConvertUTF16toUTF8(aPropertyID).get(),
         NS_ConvertUTF16toUTF8(aValue).get()));
  }
#endif

  NS_ENSURE_TRUE(valid, NS_ERROR_ILLEGAL_VALUE);

  nsString sortable;
  rv = propertyInfo->MakeSortable(aValue, sortable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString searchable;
  rv = propertyInfo->MakeSearchable(aValue, searchable);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Find all properties whose secondary sort depends on this
  // property
  nsCOMPtr<sbIPropertyArray> dependentProperties;
  rv = mPropertyManager->GetDependentProperties(aPropertyID, 
            getter_AddRefs(dependentProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 dependentPropertyCount;
  rv = dependentProperties->GetLength(&dependentPropertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PutValue(propertyDBID, aValue, searchable, sortable);
  NS_ENSURE_SUCCESS(rv, rv);

  PR_Lock(mDirtyLock);

  PRUint32 previousDirtyCount = mDirty.Count();
  
  // Mark the property that changed as dirty
  mDirty.PutEntry(propertyDBID);
  
  // Also mark as dirty any properties that use
  // the changed property in their secondary sort values
  if (dependentPropertyCount > 0) {
    for (PRUint32 i = 0; i < dependentPropertyCount; i++) {
      nsCOMPtr<sbIProperty> property;
      rv = dependentProperties->GetPropertyAt(i, getter_AddRefs(property));
      NS_ASSERTION(NS_SUCCEEDED(rv),
          "Property cache failed to update dependent properties!");
      if (NS_SUCCEEDED(rv)) {
        nsString propertyID;
        rv = property->GetId(propertyID);
        NS_ASSERTION(NS_SUCCEEDED(rv), 
          "Property cache failed to update dependent properties!");
        if (NS_SUCCEEDED(rv)) {
          mDirty.PutEntry(mCache->GetPropertyDBIDInternal(propertyID));
        }
      }
    }
  }
  
  PR_Unlock(mDirtyLock);

  // If this bag just became dirty, then let the property cache know.
  // Only notify once in order to avoid unnecessarily locking the 
  // property cache 
  if (previousDirtyCount == 0) {
    rv = mCache->AddDirty(mGuid, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If this property is user editable we need to
  // set the updated timestamp.  We only
  // track updates to user editable properties
  // since that's all the user cares about.
  PRBool userEditable = PR_FALSE;
  rv = propertyInfo->GetUserEditable(&userEditable);
  NS_ENSURE_SUCCESS(rv, rv);
  if (userEditable) {
#ifdef DEBUG
    // #updated is NOT user editable, so we won't die a firey
    // recursive death here, but lets assert just in case.
    NS_ENSURE_TRUE(!aPropertyID.EqualsLiteral(SB_PROPERTY_UPDATED),
                   NS_ERROR_UNEXPECTED);

#endif
    sbAutoString now((PRUint64)(PR_Now()/PR_MSEC_PER_SEC));
    rv = SetProperty(NS_LITERAL_STRING(SB_PROPERTY_UPDATED), now);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP sbLocalDatabaseResourcePropertyBag::Write()
{
  nsresult rv = NS_OK;
  
  if(mDirty.Count() > 0) {
    rv = mCache->Write();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult
sbLocalDatabaseResourcePropertyBag::PutValue(PRUint32 aPropertyID,
                                             const nsAString& aValue,
                                             const nsAString& aSearchable,
                                             const nsAString& aSortable)
{
  nsAutoPtr<sbPropertyData> data(new sbPropertyData(aValue, 
                                                    aSearchable, 
                                                    aSortable));
  PRBool success = mValueMap.Put(aPropertyID, data);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  data.forget();

  return NS_OK;
}

nsresult
sbLocalDatabaseResourcePropertyBag::EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc,
                                                   void *aClosure,
                                                   PRUint32 *aDirtyCount)
{
  NS_ENSURE_ARG_POINTER(aClosure);
  NS_ENSURE_ARG_POINTER(aDirtyCount);

  *aDirtyCount = mDirty.EnumerateEntries(aEnumFunc, aClosure);
  return NS_OK;
}

nsresult
sbLocalDatabaseResourcePropertyBag::ClearDirty()
{
  nsAutoLock lockDirty(mDirtyLock);

  mDirty.Clear();

  return NS_OK;
}


sbPropertyData::sbPropertyData(const nsAString& aValue, 
                               const nsAString& aSearchable, 
                               const nsAString& aSortable) :
  value(aValue),
  sortableValue(aSortable),
  searchableValue(aSearchable)                              
{
}

nsresult sbPropertyData::GetValue(nsAString &aValue) {
  aValue = value;
  return NS_OK;
}

nsresult sbPropertyData::GetSortableValue(nsAString &aSortableValue) {
  aSortableValue = sortableValue;
  return NS_OK;
}

nsresult sbPropertyData::GetSearchableValue(nsAString &aSearchableValue) {
  aSearchableValue = searchableValue;
  return NS_OK;
}
