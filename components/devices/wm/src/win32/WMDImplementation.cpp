/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright� 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the �GPL�).
// 
// Software distributed under the License is distributed 
// on an �AS IS� basis, WITHOUT WARRANTY OF ANY KIND, either 
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

#pragma once

#include "WMDevice.h"
#include "WMDImplementation.h"

#include "mswmdm_i.c"

#include "mswmdm.h"
#include "wmsdk.h"

#include "mswmdm.h"
#include "wmsdk.h"

#include "sbIDatabaseQuery.h"
#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

#include "nspr.h"
#include <xpcom/nscore.h>
#include <xpcom/nsCOMPtr.h>
#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <xpcom/nsILocalFile.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <string/nsStringAPI.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsMemory.h>
#include <unicharutil/nsUnicharUtils.h>

EXTERN_GUID(CLSID_WMDRMDeviceApp, 0x5C140836,0x43DE,0x11d3,0x84,0x7D,0x00,0xC0,0x4F,0x79,0xDB,0xC0);
EXTERN_GUID(IID_IWMDRMDeviceApp, 0x93AFDB44,0xB1E1,0x411d,0xb8,0x9b,0x75,0xad,0x4f,0x97,0x88,0x2b);
EXTERN_GUID(IID_IWMDRMDeviceApp2, 0x600D6E55,0xDEA5,0x4e4c,0x9c,0x3a,0x6b,0xd6,0x42,0xa4,0x5b,0x9d);

// Note: This certificate would allow you to transfer clear content only.
// To transfer content protected with Microsoft DRM, you need to obtain a valid certificate from Microsoft.
//

BYTE abPVK[] = {
  0x00
};
BYTE abCert[] = {
  0x00
};

#define CHECK_POINTER_VALUE(ptr) {if((ptr)==NULL)return PR_FALSE;}
#define SAFE_RELEASE(ptr) {if((ptr)!=NULL)(ptr)->Release(); (ptr)=NULL;}
#define SAFE_DELETE(ptr) {if((ptr)!=NULL)delete (ptr); (ptr)=NULL;}

using namespace std;

#define ALL_CONTENT L"All Content"
#define MUSIC       L"music"

#define DEVICE_STRING         NS_LITERAL_STRING("wmdevice.name").get()
#define DEVICE_STRING_PREFIX  NS_LITERAL_STRING(" (").get()
#define DEVICE_STRING_SUFFIX  NS_LITERAL_STRING(":)").get()
#define TRACK_TABLE           NS_LITERAL_STRING("WMDeviceTracks").get()

struct 
{
  char*   FileType;
  LPWSTR  MimeType;
} const MediaFileTypeToMimeFormatMappings[] = 
{
  {"wma;",                L"audio/x-ms-wma;"},
  {"mp3;",                L"audio/mp3;"},
  {"mp4;mpeg;m4a;",       L"audio/mpeg;"},
  {"wav;wave;",           L"audio/x-wav;"},
  {"aif;aiff;",           L"audio/x-aiff;"}
};
const PRUint32 NumPredefinedMimeFormats = sizeof(MediaFileTypeToMimeFormatMappings)/sizeof(MediaFileTypeToMimeFormatMappings[0]);

PRUint32 WMDeviceTrack::mNextTrackID = 0;

#define SB_CHECK_FAILURE(_rv)                             \
  PR_BEGIN_MACRO                                          \
  if (NS_FAILED(rv)) {                                  \
  NS_WARNING("*** SB_CHECK_FAILURE ***");             \
  /* return PR_FALSE; /**/                            \
  }                                                     \
  PR_END_MACRO

WMDeviceTrack::WMDeviceTrack():
  mIWMDMStorage(NULL),
  mTrackID(-1),
  mYear(0),
  mSelected(false),
  mSize(0),
  mPlayTime(0)
{
}

WMDeviceTrack::~WMDeviceTrack()
{
  SAFE_RELEASE(mIWMDMStorage);
}

PRBool WMDeviceTrack::AttachStorage(IWMDMStorage *pIWMDMStorage, WMDeviceFolder* pParentWMDFolder, DWORD deviceType)
{
  CHECK_POINTER_VALUE(pIWMDMStorage);

  mIWMDMStorage = pIWMDMStorage;
  WCHAR   wsz[MAX_PATH];
  if (SUCCEEDED(mIWMDMStorage->GetName(wsz, sizeof(wsz)/sizeof(WCHAR) - 1)))
    mName = wsz;

  mIWMDMStorage->GetSize((DWORD *)&mSize, ((DWORD *)&mSize)+1);

  // We use the parent folder's name to fill the album and artist info
  // for those devices that refuse to give us metadata.
  if ((deviceType & WMDM_DEVICE_TYPE_VIEW_PREF_METADATAVIEW) == 0 && pParentWMDFolder)
  {
    mAlbum = pParentWMDFolder->GetName();
    if (pParentWMDFolder->GetParentFolder())
      mArtist = pParentWMDFolder->GetParentFolder()->GetName();
  }


  IWMDMStorage3 *pIWMDMStorage3 = NULL;
  if (SUCCEEDED(mIWMDMStorage->QueryInterface(IID_IWMDMStorage3, 
    (void **) &pIWMDMStorage3)))
  {
    IWMDMMetaData   *pIWMDMMetaData = NULL;
    UINT            itemCount = 0;

    if (SUCCEEDED(pIWMDMStorage3->GetMetadata(&pIWMDMMetaData)) && 
      SUCCEEDED(pIWMDMMetaData->GetItemCount(&itemCount)))
    {
      WMDM_TAG_DATATYPE   type;
      BYTE                *value = NULL;
      UINT                size = 0;
      WCHAR*              name = NULL;

      while ((itemCount --) > 0)
      {

        if (!SUCCEEDED(pIWMDMMetaData->QueryByIndex((itemCount), (WCHAR **) &name, &type, &value, &size)))
          continue;

        switch (type)
        {
        case WMDM_TYPE_STRING:
          if(0 == wcscmp(g_wszWMDMPersistentUniqueID, name))
          {
            CoTaskMemFree(value);
          }
          else
          if(0 == wcscmp(g_wszWMDMTitle, name))
          {
            mName = (PRUnichar *)value;
            CoTaskMemFree(value);
          }
          else
          if(0 == wcscmp(g_wszWMDMAlbumTitle, name))
          {
            mAlbum = (PRUnichar *)value;
            CoTaskMemFree(value);
          }
          else
          if(0 == wcscmp(g_wszWMDMGenre, name))
          {
            mGenre = (PRUnichar *)value;
            CoTaskMemFree(value);
          }
          else
          if(0 == wcscmp(g_wszWMDMAuthor, name))
          {
            mArtist = (PRUnichar *)value;
            CoTaskMemFree(value);
          }
          else
          if(0 == wcscmp(g_wszWMDMYear, name))
          {
            PRInt32 errorCode;
            mYear = (nsString((PRUnichar *) value)).ToInteger(&errorCode);
            CoTaskMemFree(value);
          }
          break;
        case WMDM_TYPE_DWORD:
        case WMDM_TYPE_WORD:
        case WMDM_TYPE_QWORD:
          {
            if(0 == wcscmp(g_wszWMDMDuration, name))
            {
              // Media duration, in 100 ns units
              mPlayTime = *(PRUint32 *)value/(10000000);
              CoTaskMemFree(value);
            }
            else
              if(0 == wcscmp(g_wszWMDMTrack, name))
              {
                // Disc Trck ID
                mDiscTrackNumber = *(PRUint32 *)value;
                CoTaskMemFree(value);
              }
              break;
          }
        }
      }
    }
    SAFE_RELEASE(pIWMDMMetaData);
    SAFE_RELEASE(pIWMDMStorage3);
  }

  // This is a hack, since I don't know of a reliable way to identify an audio track yet
  if (mPlayTime == 0)
    return PR_FALSE;

  mTrackID = GetNextTrackID();

  return PR_TRUE;
}

//**********************************************
//WMDFolder    class related definitions
//**********************************************

WMDeviceFolder::WMDeviceFolder(WMDeviceFolder* pParentWMDFolder):
  mIWMDMStorage(NULL),
  mFolderID(0),
  mTotalPlayTimeForAllMediaTracks(0),
  mTotalPlayTimeForFolder(0),
  mParentWMDFolder(pParentWMDFolder)
{
}

WMDeviceFolder::~WMDeviceFolder()
{
  for (list<WMDeviceFolder*>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
  {
    SAFE_DELETE(*folderIterator);
  }

  for (list<WMDeviceTrack*>::iterator trackIterator = mTracks.begin(); trackIterator != mTracks.end();trackIterator ++)
  {
    SAFE_DELETE(*trackIterator);
  }

  SAFE_RELEASE(mIWMDMStorage);
}

PRBool WMDeviceFolder::AddSubFolder(WMDeviceFolder* subFolder)
{
  mSubFolders.push_back(subFolder);

  return true;
}

nsString& WMDeviceFolder::GetName() 
{
  return mName;
}

PRUint32 WMDeviceFolder::GetNumSubFolders()
{
  return mSubFolders.size();
}

WMDeviceFolder* WMDeviceFolder::GetFolder(PRUint32 index)
{
  if (GetNumSubFolders() > index)
  {
    for (list<WMDeviceFolder *>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
    {
      if (index == 0)
        return (*folderIterator);
      index --;
    }
  }
  return NULL;
}

// Looks for a subfolder with the given name and if not, creates one by that name
WMDeviceFolder* WMDeviceFolder::GetSubFolderByName(const nsString& folderName, bool bCreate)
{
  for (list<WMDeviceFolder *>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
  {
    if (folderName == (*folderIterator)->GetName())
      return (*folderIterator);
  }

  if (bCreate)
  {
    // Did not find the folder, and so
    // Create a new folder by this name.
    IWMDMStorageControl* storageControl = NULL;
    HRESULT hr = mIWMDMStorage->QueryInterface(IID_IWMDMStorageControl, (void **)&storageControl);
    if (SUCCEEDED(hr))
    {
      IWMDMStorage* newTargetStorageInterface = NULL;
      if (SUCCEEDED(storageControl->Insert(WMDM_MODE_BLOCK | WMDM_STORAGECONTROL_INSERTINTO | WMDM_CONTENT_FOLDER, 
        (PRUnichar *) folderName.get(),
        NULL,
        NULL,
        &newTargetStorageInterface)))
      {
        SAFE_RELEASE(storageControl);
        WMDeviceFolder* targetFolder = new WMDeviceFolder(this);
        if (targetFolder)
        {
          targetFolder->AttachStorage(newTargetStorageInterface);
          SAFE_RELEASE(newTargetStorageInterface);

          // Add this new folder to the parent
          AddSubFolder(targetFolder);

          return targetFolder;
        }
      }
      SAFE_RELEASE(storageControl);
    }
  }

  return NULL;
}

PRUint32 WMDeviceFolder::GetNumTracks()
{
  return mTracks.size();
}

PRUint32 WMDeviceFolder::GetPlayTimeForAllMediaTracks()
{
  return mTotalPlayTimeForAllMediaTracks;
}

PRUint32 WMDeviceFolder::GetTotalMediaPlayTimeForFolder()
{
  return mTotalPlayTimeForFolder;
}

void WMDeviceFolder::AddMediaTrack(WMDeviceTrack *pmediaTrack)
{
  if (pmediaTrack)
  {
    mTracks.push_back(pmediaTrack);
  }
}

WMDeviceFolder* WMDeviceFolder::GetParentFolder()
{
  return mParentWMDFolder;
}

// Recursive folder enumerator
PRBool WMDeviceFolder::AttachStorage(IWMDMStorage* pIWMDMStorage, PRBool bRootFolder, DWORD deviceType)
{
  WCHAR   wsz[MAX_PATH];

  CHECK_POINTER_VALUE(pIWMDMStorage);
  mIWMDMStorage = pIWMDMStorage;

  if (SUCCEEDED(mIWMDMStorage->GetName(wsz, sizeof(wsz)/sizeof(WCHAR) - 1)))
  {
    mName = wsz;
  }

  // we only want to read the ALL_CONTENT or MUSIC folder 
  nsAutoString folderNameLowerCase(mName);
  ToLowerCase(folderNameLowerCase);
  if ((deviceType & WMDM_DEVICE_TYPE_VIEW_PREF_METADATAVIEW) &&
    (!bRootFolder && (folderNameLowerCase != nsAutoString(MUSIC))))
    return PR_FALSE;

  // Check for playlists
  //IWMDMStorage4 *pIWMDMStorage4 = NULL;
  //mIWMDMStorage->QueryInterface(IID_IWMDMStorage4, (void**) &pIWMDMStorage4);
  //if (pIWMDMStorage4)
  //{
  //  DWORD refs = 0;
  //  IWMDMStorage **ppIWMDMStorage = NULL;
  //  pIWMDMStorage4->GetReferences(&refs, &ppIWMDMStorage);
  //  while (refs -- )
  //  {
  //    (*ppIWMDMStorage)->GetName(wsz, sizeof(wsz)/sizeof(WCHAR) - 1);
  //    (*ppIWMDMStorage) ++;
  //  }
  //}
  // Enumerate the children/contents
  IWMDMEnumStorage *pIWMDMEnumStorage = NULL;
  mIWMDMStorage->EnumStorage(&pIWMDMEnumStorage);
  if (pIWMDMEnumStorage)
  {
    IWMDMStorage*   pChildIWMDMStorage = NULL;
    ULONG           numFetched = 0;
    while (SUCCEEDED(pIWMDMEnumStorage->Next(1, &pChildIWMDMStorage, &numFetched)) &&
      numFetched > 0 )
    {
      if (pChildIWMDMStorage)
      {
        DWORD           dStorageAttribute = 0;
        _WAVEFORMATEX   wx;
        pChildIWMDMStorage->GetAttributes(&dStorageAttribute, &wx);
        if (dStorageAttribute & WMDM_FILE_ATTR_FOLDER)
        {
          WMDeviceFolder *pFolder = new WMDeviceFolder(this);
          if (pFolder)
          {
            if (pFolder->AttachStorage(pChildIWMDMStorage, false, deviceType) >= 0)
            {
              mSubFolders.push_back(pFolder);
              mTotalPlayTimeForAllMediaTracks += pFolder->GetPlayTimeForAllMediaTracks();
            }
            else
              delete pFolder;
          }
        }
        else if ((dStorageAttribute & WMDM_FILE_ATTR_FILE) || 
                (dStorageAttribute & WMDM_FILE_ATTR_AUDIO) ||
                (dStorageAttribute & WMDM_FILE_ATTR_CANPLAY) ||
                (dStorageAttribute & WMDM_FILE_ATTR_MUSIC))
        {

          WMDeviceTrack* pMediaTrack = new WMDeviceTrack;
          if (pMediaTrack)
          {
            if (pMediaTrack->AttachStorage(pChildIWMDMStorage, this, deviceType))
            {
              mTracks.push_back(pMediaTrack);
              mTotalPlayTimeForAllMediaTracks += pMediaTrack->mPlayTime;
            }
            else
              delete pMediaTrack;
          }
        }
      }
    }
    SAFE_RELEASE(pIWMDMEnumStorage);
  }

  return PR_TRUE;
}

PRUint64 WMDeviceFolder::GetFreeStorageSize()
{
  PRUint64 totalFreeSize = 0;
  IWMDMStorageGlobals *pStorageGlobals = NULL;
  if (SUCCEEDED(mIWMDMStorage->GetStorageGlobals(&pStorageGlobals)))
  {
    pStorageGlobals->GetTotalFree(  (DWORD *)&totalFreeSize, 
      ((DWORD *)&totalFreeSize)+1);
    SAFE_RELEASE(pStorageGlobals);
  }
  return totalFreeSize;
}

PRUint64 WMDeviceFolder::GetTotalStorageSize()
{
  PRUint64 totalStorageSize = 0;
  IWMDMStorageGlobals *pStorageGlobals = NULL;
  if (SUCCEEDED(mIWMDMStorage->GetStorageGlobals(&pStorageGlobals)))
  {
    pStorageGlobals->GetTotalSize(  (DWORD *)&totalStorageSize, 
      ((DWORD *)&totalStorageSize)+1);
    SAFE_RELEASE(pStorageGlobals);
  }
  return totalStorageSize;
}

// Grand total of all media tracks (including the nested sub-folders)
PRUint32 WMDeviceFolder::GetNumTracksIncSubFolders()
{
  PRUint32 numtracks = 0;
  for (list<WMDeviceFolder*>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
  {
    numtracks += (*folderIterator)->GetNumTracksIncSubFolders();
  }
  numtracks += mTracks.size();

  return numtracks;
}

WMDeviceTrack* WMDeviceFolder::GetMediaTrackIncSubFolders(PRUint32 index)
{
  // Lets check the media track list within this folder first
  PRUint32 currentnumtracks = GetNumTracks();
  if (currentnumtracks > 0)
  {
    if (index < currentnumtracks)
      return GetMediaTrack(index);
  }

  // Now check the subfolders
  for (list<WMDeviceFolder *>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
  {
    WMDeviceFolder* current = (*folderIterator);
    currentnumtracks += current->GetNumTracksIncSubFolders();
    if (index < currentnumtracks)
    {
      // The track we are looking for is within this folder
      PRUint32 indexwithinsubfolder = 
        index - (currentnumtracks - current->GetNumTracksIncSubFolders());
      return current->GetMediaTrackIncSubFolders(indexwithinsubfolder);
    }
  }
  return NULL;
}

PRBool WMDeviceFolder::DeleteMediaTrackByTrackID(PRUint32 mediaTrackID)
{
  PRBool retval = PR_FALSE;

  // Check in our folder if this track exists
  for (list<WMDeviceTrack *>::iterator trackIterator = mTracks.begin(); trackIterator != mTracks.end();trackIterator ++)
  {
    WMDeviceTrack* current = (*trackIterator);
    if (current->mTrackID == mediaTrackID)
    {
      mTracks.erase(trackIterator);
      SAFE_DELETE(current);
      retval = PR_TRUE;
      return retval;
    }
  }

  // Did not find the track in our folder, so check in the subfolders
  for (list<WMDeviceFolder *>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
  {
    if (0 == (*folderIterator)->DeleteMediaTrackByTrackID(mediaTrackID))
    {
      retval = PR_TRUE;
      break;
    }
  }

  return retval;
}

WMDeviceTrack* WMDeviceFolder::GetMediaTrack(PRUint32 index)
{
  if (GetNumTracks() > index)
  {
    for (list<WMDeviceTrack *>::iterator trackIterator = mTracks.begin(); trackIterator != mTracks.end();trackIterator ++)
    {
      if (index == 0)
        return (*trackIterator);
      index --;
    }
  }
  return NULL;
}

WMDeviceTrack* WMDeviceFolder::GetMediaTrackByID(PRUint32 mediaTrackID)
{
  for (list<WMDeviceTrack *>::iterator trackIterator = mTracks.begin(); trackIterator != mTracks.end();trackIterator ++)
  {
    if ((*trackIterator)->mTrackID == mediaTrackID)
      return (*trackIterator);
  }
  return NULL;
}

WMDeviceTrack* WMDeviceFolder::GetMediaTrackByTrackIDIncSubFolders(PRUint32 mediaTrackID)
{
  WMDeviceTrack* pMediaTrack = GetMediaTrackByID(mediaTrackID);
  if (!pMediaTrack)
  {
    for (list<WMDeviceFolder *>::iterator folderIterator = mSubFolders.begin(); folderIterator != mSubFolders.end();folderIterator ++)
    {
      pMediaTrack = (*folderIterator)->GetMediaTrackByTrackIDIncSubFolders(mediaTrackID);
      if (pMediaTrack)
        break;
    }
  }

  return pMediaTrack;
}

// WMDevice class definitions
//
PRUint32 WMDevice::mDeviceNumber = 0;

WMDevice::WMDevice(class sbWMDObjectManager* parent, IWMDMDevice* pIDevice):
  mIWMDevice(pIDevice),
  mIWMRootStorage(NULL),
  mRootFolder(NULL),
  mDeviceTracksTable(TRACK_TABLE)
{
  mStopTransfer = PR_FALSE;
  mDeviceState = 0;
  mCurrentTransferRowNumber = -1;
  mNumTracks = 0;
  mUsedSpace = 0;
  mFreeSpace = 0;
  mSupportsTetheredDownload = PR_FALSE;
}

WMDevice::~WMDevice()
{
  CoTaskMemFree(mWaveFormatEx);
  CoTaskMemFree(mMimeFormats);

  SAFE_RELEASE(mIWMDevice);
}

PRBool WMDevice::Initialize()
{
  PRBool retVal = PR_FALSE;


  if (mIWMDevice == NULL)
    return retVal;

  ULONG               numfetched = 0;
  IWMDMEnumStorage    *pEnumRootStorage = NULL;
  
  mIWMDevice->GetType(&mDeviceType);

  PRUnichar tempBuf[MAX_PATH+1];
  mIWMDevice->GetName(tempBuf, MAX_PATH);
  mDeviceName = tempBuf;

  mIWMDevice->GetFormatSupport(&mWaveFormatEx,
                                  &mNumFormats,
                                  &mMimeFormats,
                                  &mNumMimeFormats);

  if (!IsMediaPlayer())
    return PR_FALSE;

  if (SUCCEEDED(mIWMDevice->EnumStorage(&pEnumRootStorage )))
  {
    if (SUCCEEDED(pEnumRootStorage->Next(1, &mIWMRootStorage, &numfetched)) &&
      numfetched > 0)
    {
     if (ReadDeviceAttributes() == PR_TRUE)
      {
        mSupportsTetheredDownload = IsTetheredDownloadCapable();
        retVal = PR_TRUE;
      }
    }
    SAFE_RELEASE(pEnumRootStorage);
  }

  if (retVal == PR_TRUE)
  {
    // Get the string bundle for our strings
    if ( !mStringBundle.get() )
    {
      nsresult rv = NS_OK;
      nsIStringBundleService *  StringBundleService = nsnull;
      rv = CallGetService("@mozilla.org/intl/stringbundle;1", &StringBundleService );
      if ( NS_SUCCEEDED(rv) )
      {
        rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties", getter_AddRefs(mStringBundle) );
        StringBundleService->Release();
      }
    }

    PRUnichar *value = nsnull;
    mStringBundle->GetStringFromName(DEVICE_STRING, &value);
    mDeviceString = value;
    PR_Free(value);
    mDeviceString += DEVICE_STRING_PREFIX;
    mDeviceString += mDeviceName;
    mDeviceString += DEVICE_STRING_SUFFIX;

    mDeviceContext.AssignLiteral(CONTEXT_WINDOWS_MEDIA_DEVICE);
    mDeviceContext += mDeviceNumber ++;

    EnumTracks();
    ClearLibraryData();
    UpdateDeviceLibraryData();
  }

  return retVal;
}

PRBool WMDevice::Finalize()
{

  ClearLibraryData();

  return PR_TRUE;
}

void WMDevice::EnumTracks()
{
  mRootFolder.AttachStorage(mIWMRootStorage, true, mType);
  mFreeSpace = (PRUint32) mRootFolder.GetFreeStorageSize();
  mUsedSpace = (PRUint32) (mRootFolder.GetTotalStorageSize() - mFreeSpace);
  mNumTracks = (PRUint32) mRootFolder.GetNumTracksIncSubFolders();
}

nsString& WMDevice::GetTracksTable()
{
  return mDeviceTracksTable;
}

nsString& WMDevice::GetDeviceString()
{
  return mDeviceString;
}

nsString& WMDevice::GetDeviceContext()
{
  return mDeviceContext;
}

PRBool WMDevice::UpdateDeviceLibraryData()
{
  nsresult rv;
  PRBool bRet = PR_FALSE;
  if (mNumTracks)
  {
    nsCOMPtr<sbIDatabaseQuery> pQuery =
      do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
    SB_CHECK_FAILURE(rv);

    rv = pQuery->SetAsyncQuery(PR_FALSE);
    SB_CHECK_FAILURE(rv);

    rv = pQuery->SetDatabaseGUID(GetDeviceContext());
    SB_CHECK_FAILURE(rv);

    nsCOMPtr<sbIMediaLibrary> pLibrary =
      do_CreateInstance("@songbirdnest.com/Songbird/MediaLibrary;1", &rv);
    SB_CHECK_FAILURE(rv);

    rv = pLibrary->SetQueryObject(pQuery);
    SB_CHECK_FAILURE(rv);

    rv = pLibrary->CreateDefaultLibrary();
    SB_CHECK_FAILURE(rv);

    nsCOMPtr<sbIPlaylistManager> pPlaylistManager =
      do_CreateInstance("@songbirdnest.com/Songbird/PlaylistManager;1", &rv);
    SB_CHECK_FAILURE(rv);

    rv = pPlaylistManager->CreateDefaultPlaylistManager(pQuery);
    SB_CHECK_FAILURE(rv);

    nsAutoString strDevice(GetDeviceString());
    nsCOMPtr<sbIPlaylist> pPlaylist;
    rv = pPlaylistManager->CreatePlaylist(GetTracksTable(), strDevice,
      strDevice, NS_LITERAL_STRING("wmd"),
      pQuery, getter_AddRefs(pPlaylist));
    SB_CHECK_FAILURE(rv);

    rv = pQuery->Execute(&bRet);
    SB_CHECK_FAILURE(rv);

    rv = pQuery->ResetQuery();
    SB_CHECK_FAILURE(rv);

    // Create record for each track
    for (unsigned int trackNum = 0; trackNum < mNumTracks; trackNum ++)
    {
      WMDeviceTrack* currentTrack = mRootFolder.GetMediaTrackByTrackIDIncSubFolders(trackNum);
      if (currentTrack == NULL)
        continue;

      const static PRUnichar *aMetaKeys[] =
                    {NS_LITERAL_STRING("length").get(), 
                    NS_LITERAL_STRING("artist").get(),
                    NS_LITERAL_STRING("title").get(),
                    NS_LITERAL_STRING("album").get(),
                    NS_LITERAL_STRING("genre").get(),
                    NS_LITERAL_STRING("year").get(),
                    };
      const static PRUint32 nMetaKeyCount =
        sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

      PRUnichar formattedLength[10];
      wsprintf(formattedLength, L"%d:%02d", currentTrack->mPlayTime/60, currentTrack->mPlayTime%60);
      nsString strLength(formattedLength);

      PRUnichar** aMetaValues =
        (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));

      aMetaValues[0] = ToNewUnicode(strLength);
      aMetaValues[1] = ToNewUnicode(currentTrack->mArtist);
      aMetaValues[2] = ToNewUnicode(currentTrack->mName);
      aMetaValues[3] = ToNewUnicode(currentTrack->mAlbum);
      aMetaValues[4] = ToNewUnicode(currentTrack->mGenre);

      PRUnichar formattedYear[10];
      wsprintf(formattedYear, L"%d", currentTrack->mYear);
      aMetaValues[5] = ToNewUnicode(nsString(formattedYear));

      // This should be an unicode
      nsAutoString guid;
      nsString url = currentTrack->mArtist;
      url += currentTrack->mName;
      url += currentTrack->mAlbum;
      url += currentTrack->mGenre;

      rv = pLibrary->AddMedia(url, nMetaKeyCount, aMetaKeys, nMetaKeyCount,
        const_cast<const PRUnichar **>(aMetaValues),
        PR_TRUE, PR_FALSE, guid);
      SB_CHECK_FAILURE(rv);

      // Don't know why (thanks to the in-ability to debug javascript) but
      // sometimes "addMedia" fails the first time and calling it again
      // succeeds, needs to be investigated.
      if (guid.IsEmpty()) {
        rv = pLibrary->AddMedia(url, nMetaKeyCount, aMetaKeys, nMetaKeyCount,
          const_cast<const PRUnichar **>(aMetaValues),
          PR_FALSE, PR_FALSE, guid);
        SB_CHECK_FAILURE(rv);
      }

      if(!guid.IsEmpty() && pPlaylist) {
        rv = pPlaylist->AddByGUID(guid, GetDeviceContext(), -1, PR_FALSE,
          PR_FALSE, &bRet);
        SB_CHECK_FAILURE(rv);
      }

      nsMemory::Free(aMetaValues[0]);
      nsMemory::Free(aMetaValues[1]);
      nsMemory::Free(aMetaValues[2]);
      nsMemory::Free(aMetaValues[3]);
      nsMemory::Free(aMetaValues[4]);
      nsMemory::Free(aMetaValues[5]);
      nsMemory::Free(aMetaValues);
    }
    rv = pQuery->Execute(&bRet);
    SB_CHECK_FAILURE(rv);

  }
  else
  {
    // Remove all the info about this CD
    ClearLibraryData();
  }

  return PR_TRUE;
}

// Clears any previous entries in the database for this WM Drive.
void WMDevice::ClearLibraryData()
{
  nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  pQuery->SetAsyncQuery(PR_FALSE);
  pQuery->SetDatabaseGUID(GetDeviceContext());

  PRBool retVal;

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
  nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbirdnest.com/Songbird/MediaLibrary;1" );

  if(pPlaylistManager.get())
  {
    pPlaylistManager->DeletePlaylist(GetTracksTable(), pQuery, &retVal);

    if(pLibrary.get())
    {
      pLibrary->SetQueryObject(pQuery);
      pLibrary->PurgeDefaultLibrary(PR_FALSE, &retVal);
    }
  }
}

PRBool WMDevice::IsTetheredDownloadCapable()
{
  IWMDRMDeviceApp2* pDeviceApp2 = NULL;

  HRESULT hr = CoCreateInstance(CLSID_WMDRMDeviceApp,
    NULL,
    CLSCTX_ALL,
    IID_IWMDRMDeviceApp2,
    (void**)&pDeviceApp2 );

  if (SUCCEEDED(hr) && pDeviceApp2 && mIWMDevice)
  {
    DWORD status = 0;
    pDeviceApp2->QueryDeviceStatus2(mIWMDevice, WMDRM_QUERY_DEVICE_ISWMDRM, &status);
    SAFE_RELEASE(pDeviceApp2);

    if (status & WMDRM_DEVICE_ISWMDRM)
      return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool WMDevice::IsFileFormatAcceptable(const char* fileFormat)
{
  char* extension = strrchr(fileFormat, '.');
  if (extension == NULL || *(++extension) == NULL)
    return PR_FALSE;

  _strlwr(extension);

  // This is a workaround for wave files
  // Since some devices don't report wave file
  // format as supported, we are assuming we
  // can download wave files for any device.
  if ((strcmp(extension, "wav") == NULL) || (strcmp(extension, "wave") == NULL))
    return true;

  // Get the equivalent stock mime format map info
  for (PRUint32 index = 0; index < NumPredefinedMimeFormats; index ++)
  {
    if (strstr(MediaFileTypeToMimeFormatMappings[index].FileType, extension))
      break;
  }

  if (index < NumPredefinedMimeFormats)
  {
    for (PRUint32 i = 0; i < mNumMimeFormats; i ++)
    {
      if (wcsstr(MediaFileTypeToMimeFormatMappings[index].MimeType, mMimeFormats[i]))
      {
        // Found a match and the device can accept this file
        return true;
      }
    }
  }

  return false;
}
// Function tests if the discovered device is a media player
// device or not using some creative ways, check for codecs,
// check for mp3 and wma as acceptable file formats for playback :).
PRBool WMDevice::IsMediaPlayer()
{
  // Check if this really is a media device
  if (mNumFormats == 0) // No codecs so not a player
    return PR_FALSE;

  if ((mType & (WMDM_DEVICE_TYPE_PLAYBACK | 
      WMDM_DEVICE_TYPE_RECORD | WMDM_DEVICE_TYPE_DECODE | WMDM_DEVICE_TYPE_VIEW_PREF_METADATAVIEW)) == 0)
  {
    if (!IsFileFormatAcceptable("wma"))
      if (!IsFileFormatAcceptable("mp3")) 
        return PR_FALSE; 
  }
  return PR_TRUE;
}

PRBool WMDevice::ReadDeviceAttributes(void)
{
  WCHAR wsz[MAX_PATH+1];

  // Device name
  if (SUCCEEDED(mIWMDevice->GetName(wsz, sizeof(wsz)/sizeof(WCHAR) - 1)))
  {
    mDeviceName = wsz;
  }

  mIWMDevice->GetType((DWORD *) &mType);

  BYTE abMAC[SAC_MAC_LEN];
  mIWMDevice->GetSerialNumber(&mSerialNumber, (BYTE*)abMAC);


  // Get device manufacturer
  if(SUCCEEDED(mIWMDevice->GetManufacturer(wsz, sizeof(wsz)/sizeof(WCHAR) - 1 )))
  {
    mManufacturer = wsz;
  }

  IWMDMDevice2* pIDevice2 = NULL;
  if (SUCCEEDED(mIWMDevice->QueryInterface(IID_IWMDMDevice2, (void **) &pIDevice2)))
  {
    WCHAR wszCanonicalName[MAX_PATH];
    if (SUCCEEDED(pIDevice2->GetCanonicalName(wszCanonicalName, 
      (sizeof(wszCanonicalName)/sizeof(wszCanonicalName[0])))))
    {
      mDeviceCanonicalName = wszCanonicalName;
    }
    SAFE_RELEASE(pIDevice2);
  }

  return PR_TRUE;
}

nsString WMDevice::GetDeviceName()
{
  return mDeviceName;
}

nsString WMDevice::GetDeviceCanonincalName()
{
  return mDeviceCanonicalName;
}

PRUint32 WMDevice::GetUsedSpace()
{
  return mUsedSpace;
}

PRUint32 WMDevice::GetAvailableSpace()
{
  return mFreeSpace;
}

nsString& WMDevice::GetDeviceTracksTable()
{
  return mDeviceTracksTable;
}

void WMDevice::EvaluateDevice()
{
}

void WMDevice::UpdateIOProgress(DBInfoForTransfer *ripInfo)
{
}

PRBool WMDevice::UploadTrack(const PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 )
{
  PRBool retVal = PR_FALSE;

  return retVal;
}

PRBool WMDevice::UploadTable(const PRUnichar* table)
{
  PRBool retVal = PR_FALSE;

  return retVal;
}

void WMDevice::StopTransfer()
{
}

void WMDevice::TransferComplete()
{
}

void WMDevice::MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure)
{
}

PRBool WMDevice::IsDownloadSupported()
{
  return PR_TRUE;
}

void WMDevice::SuspendTransfer()
{
}

void WMDevice::ResumeTransfer()
{
}


// ********************************
// CWMDProgress related definitions
// ********************************

HRESULT CWMDProgress::Begin(DWORD dwEstimatedTicks)
{
  this->OnBegin(dwEstimatedTicks);
  return S_OK;
}
HRESULT CWMDProgress::Progress(DWORD dwTranspiredTicks)
{
  if (!this->OnProgress(dwTranspiredTicks))
    return WMDM_E_USER_CANCELLED;
  return S_OK;
}
HRESULT CWMDProgress::End()
{
  this->OnEnd();
  return S_OK;
}

// IUnknown impl

STDMETHODIMP CWMDProgress::QueryInterface(REFIID riid, void ** ppvObject)
{
  HRESULT hr = E_INVALIDARG;
  if (NULL != ppvObject)
  {
    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IWMDMProgress) || IsEqualIID(riid, IID_IUnknown))
    {
      *ppvObject = this;
      AddRef();
      hr = S_OK;
    }
    else
    {
      hr = E_NOINTERFACE;
    }
  }
  return hr;
}

ULONG STDMETHODCALLTYPE CWMDProgress::AddRef()
{
  return ++mdwRefCount;
}

ULONG STDMETHODCALLTYPE CWMDProgress::Release()
{
  DWORD dwRetVal = --mdwRefCount;
  if (0 == dwRetVal)
  {
  }
  return dwRetVal;
}

// WMDManager class definitions
//

WMDManager::WMDManager(class sbWMDevice* parent):
  mParentDevice(parent),
  mWMDevMgr(NULL),
  mSAC(NULL)
{
}

WMDManager::~WMDManager()
{
  for (std::list<WMDevice *>::iterator iteratorWMObjectects = mDevices.begin(); iteratorWMObjectects != mDevices.end(); iteratorWMObjectects ++)
  {
    WMDevice* wmObject =(*iteratorWMObjectects);
    delete wmObject;
  } 

  SAFE_DELETE(mSAC);
  SAFE_RELEASE(mWMDevMgr);

  CoUninitialize();
}

void WMDManager::Initialize()
{
  CoInitialize(0);
  if (AuthenticateWithWMDManager())
  {
    RegisterWithWMDForNotifications();
    EnumeratePortablePlayers();
  }
}

void WMDManager::Finalize()
{
  for (std::list<WMDevice *>::iterator iteratorWMObjectects = mDevices.begin(); iteratorWMObjectects != mDevices.end(); iteratorWMObjectects ++)
  {
    WMDevice* wmObject =(*iteratorWMObjectects);
    wmObject->Finalize();
  } 
}

WMDevice* WMDManager::GetDeviceMatchingString(const nsAString& deviceString)
{
  WMDevice* wmObject = NULL;

  for (std::list<WMDevice *>::iterator iteratorWMObjectects = mDevices.begin(); iteratorWMObjectects != mDevices.end(); iteratorWMObjectects ++)
  {
    if ((*iteratorWMObjectects)->GetDeviceString() == nsString(deviceString))
    {
      wmObject = (*iteratorWMObjectects);
      break;
    }
  } 

  return wmObject;
}

void WMDManager::GetContext(const nsAString& deviceString, nsAString& _retval)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    _retval.Assign(wmObject->GetDeviceContext());
  }
}

void WMDManager::GetDeviceStringByIndex(PRUint32 aIndex, nsAString& _retval)
{ 
  PRUint32 currentIndex = 0;
  for (std::list<WMDevice *>::iterator iteratorWMObjectects = mDevices.begin(); iteratorWMObjectects != mDevices.end(); iteratorWMObjectects ++)
  {
    if (currentIndex ++ == aIndex)
    {
      _retval.Assign((*iteratorWMObjectects)->GetDeviceString());
    }
  }
}

PRBool WMDManager::IsDownloadSupported(const nsAString& deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    retVal = wmObject->IsDownloadSupported();
  }
  return retVal;
}

PRUint32 WMDManager::GetSupportedFormats(const nsAString& deviceString)
{ 
  return 0;
}

PRBool WMDManager::IsUploadSupported(const nsAString&  deviceString)
{ 
  return PR_FALSE;
}

PRBool WMDManager::IsTransfering(const nsAString&  deviceString)
{ 
  return PR_FALSE;
}

PRBool WMDManager::IsDeleteSupported(const nsAString&  deviceString)
{ 
  return PR_TRUE;
}

PRUint32 WMDManager::GetUsedSpace(const nsAString&  deviceString)
{ 
  PRUint32 usedSpace = 0;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    usedSpace = wmObject->GetUsedSpace();
  }

  return usedSpace;
}

PRUint32 WMDManager::GetAvailableSpace(const nsAString&  deviceString)
{ 
  PRUint32 availableSpace = 0;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    availableSpace = wmObject->GetAvailableSpace();
  }

  return availableSpace;
}

PRBool WMDManager::GetTrackTable(const nsAString&  deviceString, nsAString& dbContext, nsAString& tableName)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    tableName = wmObject->GetDeviceTracksTable();
  }

  return PR_TRUE;
}

PRBool WMDManager::EjectDevice(const nsAString&  deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    retVal = wmObject->EjectDevice();
  }

  return retVal;
}
 
PRBool WMDManager::IsUpdateSupported(const nsAString&  deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    retVal = wmObject->IsUpdateSupported();
  }

  return retVal;
}

PRBool WMDManager::IsEjectSupported(const nsAString&  deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    retVal = wmObject->IsEjectSupported();
  }

  return retVal;
}

PRUint32 WMDManager::GetNumDevices()
{ 
  return mDevices.size();
}

PRUint32 WMDManager::GetDeviceState(const nsAString&  deviceString)
{ 
  return 0;
}

PRBool WMDManager::StopCurrentTransfer(const nsAString&  deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    wmObject->StopTransfer();
    retVal = PR_TRUE;
  }

  return retVal;
}

PRBool WMDManager::SuspendTransfer(const nsAString&  deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    wmObject->SuspendTransfer();
    retVal = PR_TRUE;
  }

  return retVal;
}

PRBool WMDManager::ResumeTransfer(const nsAString&  deviceString)
{ 
  PRBool retVal = PR_FALSE;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    wmObject->ResumeTransfer();
    retVal = PR_TRUE;
  }

  return retVal;
}

PRUint32 WMDManager::GetCurrentTransferRowNumber(const nsAString& deviceString)
{ 
  PRUint32 rowNumber = -1;
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    rowNumber = wmObject->GetCurrentTransferRowNumber();
  }

  return rowNumber;
}

PRBool WMDManager::UploadTable(const nsAString& DeviceString, const nsAString& TableName)
{ 
  return PR_FALSE;
}

void WMDManager::SetTransferState(const nsAString& deviceString, PRInt32 newState)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    wmObject->SetTransferState(newState);
  }
}

PRBool WMDManager::IsDeviceIdle(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsDeviceIdle();
  }

  return PR_FALSE;
}

PRBool WMDManager::IsDownloadInProgress(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsDownloadInProgress();
  }

  return PR_FALSE;
}

PRBool WMDManager::IsUploadInProgress(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsUploadInProgress();
  }

  return PR_FALSE;
}

PRBool WMDManager::IsTransferInProgress(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsTransferInProgress();
  }

  return PR_FALSE;
}

PRBool WMDManager::IsDownloadPaused(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsDownloadPaused();
  }

  return PR_FALSE;
}

PRBool WMDManager::IsUploadPaused(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsUploadPaused();
  }

  return PR_FALSE;
}

PRBool WMDManager::IsTransferPaused(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->IsTransferPaused();
  }

  return PR_FALSE;
}

void WMDManager::TransferComplete(const nsAString& deviceString)
{ 
  WMDevice* wmObject = GetDeviceMatchingString(deviceString);

  if (wmObject)
  {
    return wmObject->TransferComplete();
  }
}

// Authenticates with MS Windows Media Device manager interface
PRBool WMDManager::AuthenticateWithWMDManager()
{
  IComponentAuthenticate  *pAuth = NULL;
  HRESULT                 hrInit = NULL;

  // Acquire the authentication interface of WMDM
  hrInit = CoCreateInstance(CLSID_MediaDevMgr,
                            NULL,
                            CLSCTX_ALL,
                            IID_IComponentAuthenticate,
                            (void**)&pAuth );
  if (!SUCCEEDED(hrInit))
    return PR_FALSE;

  // Create a client authentication object
  mSAC = new CSecureChannelClient;
  if (mSAC)
  {
    // Select the certificate and the associated private key into the SAC
    if (SUCCEEDED(hrInit = mSAC->SetCertificate(SAC_CERT_V1,
                                                (BYTE *)abCert, sizeof(abCert),
                                                (BYTE *)abPVK,  sizeof(abPVK) )))
    {
      // Select the authentication interface into SAC
      mSAC->SetInterface(pAuth);
      // Authenticate with V1 protocol
      if (SUCCEEDED(hrInit = mSAC->Authenticate(SAC_PROTOCOL_V1)))
      {
        // Acquire an interface to Windows Media device manager.
        hrInit = pAuth->QueryInterface(IID_IWMDeviceManager, 
          (void**) &mWMDevMgr);
      }
    }

  }

  SAFE_RELEASE(pAuth);

  return SUCCEEDED(hrInit)?PR_TRUE:PR_FALSE;
}

PRBool WMDManager::RegisterWithWMDForNotifications()
{
  PRInt32 retval = PR_FALSE;
  IConnectionPointContainer *spICPC = NULL;
  IConnectionPoint *spICP = NULL;

  if(!SUCCEEDED(mWMDevMgr->QueryInterface(IID_IConnectionPointContainer, 
    (void**) & spICPC)))
  {
    return retval;
  }

  if (SUCCEEDED(spICPC->FindConnectionPoint(IID_IWMDMNotification, &spICP)))
  {
    DWORD dwCookie;
    if (SUCCEEDED(spICP->Advise((IUnknown*)(IWMDMNotification*) this, &dwCookie)))
    {
      mdwNotificationCookie = dwCookie;
      retval = 0;
    }
  }
  //Cleanup the COM pointers
  if(spICPC)
    spICPC->Release();

  if(spICP)
    spICP->Release();

  return retval;
}

PRBool WMDManager::EnumeratePortablePlayers(void)
{
  CHECK_POINTER_VALUE(mWMDevMgr);
  IWMDMEnumDevice* pIWMDMEnumDevice = NULL;

  // Query for device enumerator interface
  IWMDeviceManager2 *pIWMDevMgr2 = NULL;

  // Microsoft recommends using IWMDeviceManager2 extended interface for
  // for efficient processing of device enumeration, since it only loads
  // the service providers for the PnP devices that are currently connected.
  if (SUCCEEDED(mWMDevMgr->QueryInterface(IID_IWMDeviceManager2, (void**) &pIWMDevMgr2)))
  {
    pIWMDevMgr2->EnumDevices2(&pIWMDMEnumDevice);
    pIWMDevMgr2->Release();
  }
  else
  {
    // Fallback for no extended interface support
    mWMDevMgr->EnumDevices(&pIWMDMEnumDevice);  
  }
  CHECK_POINTER_VALUE(pIWMDMEnumDevice);

  IWMDMDevice*    pIDevice = NULL;
  ULONG           ulNum = 1;
  ULONG           ulNumFetched = 0;
  DWORD           pdwStatus = 0;

  while (SUCCEEDED(pIWMDMEnumDevice->Next(ulNum, &pIDevice,   &ulNumFetched)) && ulNumFetched > 0)
  {
    if(pIDevice)
      AddDevice(pIDevice);
  }

  SAFE_RELEASE(pIWMDMEnumDevice);

  return PRInt32(0);
}


PRBool WMDManager::AddDevice(IWMDMDevice* pIDevice)
{
  if (pIDevice == NULL)
    return PR_TRUE;

  WMDevice* wmdevice = new WMDevice(this, pIDevice);
  if (wmdevice->Initialize())
  {
    mDevices.push_back(wmdevice);
    // Notify listeners of the event
    mParentDevice->DoDeviceConnectCallback(wmdevice->GetDeviceString());
  }
  else
  {
    // Probably a removable drive or something posing
    // as a device.
    delete wmdevice;
  }

  return PR_FALSE;
}

PRInt32 WMDManager::RemoveDevice(LPCWSTR pCanonicalName)
{
  CHECK_POINTER_VALUE(pCanonicalName);

  for (std::list<WMDevice *>::iterator iteratorWMObjectects = mDevices.begin(); iteratorWMObjectects != mDevices.end(); iteratorWMObjectects ++)
  {
    if ((*iteratorWMObjectects)->GetDeviceCanonincalName() == nsString(pCanonicalName))
    {
      // Notify listeners of the event
      mParentDevice->DoDeviceConnectCallback((*iteratorWMObjectects)->GetDeviceString());
      delete (*iteratorWMObjectects);
      mDevices.erase(iteratorWMObjectects);

      return PR_TRUE;
    }
  } 

  return PR_FALSE;
}

PRInt32 WMDManager::RemoveDevice(WMDevice *wmdevice)
{
  CHECK_POINTER_VALUE(wmdevice);

  for (std::list<WMDevice *>::iterator iteratorWMObjectects = mDevices.begin(); iteratorWMObjectects != mDevices.end(); iteratorWMObjectects ++)
  {
    if ((*iteratorWMObjectects) == wmdevice)
    {
      delete (*iteratorWMObjectects);
      mDevices.erase(iteratorWMObjectects);

      return PR_TRUE;
    }
  } 

  return PR_FALSE;
}

void WMDManager::DeviceConnect(LPCWSTR pwszCanonicalName)
{
  IWMDeviceManager2   *spIWmdm2 = NULL;
  IWMDMDevice         *pDevice = NULL;

  if (SUCCEEDED(mWMDevMgr->QueryInterface(IID_IWMDeviceManager2, (void **) &spIWmdm2)))
  {
    if (spIWmdm2 && SUCCEEDED (spIWmdm2->GetDeviceFromCanonicalName(pwszCanonicalName, &pDevice)))
    {
      AddDevice(pDevice);
    }
    SAFE_RELEASE(spIWmdm2);
  }
}

void WMDManager::DeviceDisconnect(LPCWSTR pwszCanonicalName)
{
  RemoveDevice(pwszCanonicalName);
}

void WMDManager::MediaInsert(LPCWSTR pwszCanonicalName)
{
}

void WMDManager::MediaRemove(LPCWSTR pwszCanonicalName)
{
}

HRESULT CNotificationHandler::WMDMMessage (/*[in]*/ DWORD dwMessageType, /*[in]*/ LPCWSTR pwszCanonicalName)
{
  HRESULT hr = S_OK;

  switch (dwMessageType)
  {
  case WMDM_MSG_DEVICE_ARRIVAL:
    DeviceConnect(pwszCanonicalName);
    break;
  case WMDM_MSG_DEVICE_REMOVAL:
    DeviceDisconnect(pwszCanonicalName);
    break;
  case WMDM_MSG_MEDIA_ARRIVAL:
    MediaInsert(pwszCanonicalName);
    break;
  case WMDM_MSG_MEDIA_REMOVAL:
    MediaRemove(pwszCanonicalName);
    break;
  }

  return S_OK;
}

//IUnknown impl

STDMETHODIMP CNotificationHandler::QueryInterface(REFIID riid, void ** ppvObject)
{
  HRESULT hr = E_INVALIDARG;
  if (NULL != ppvObject)
  {
    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IWMDMNotification) || IsEqualIID(riid, IID_IUnknown))
    {
      *ppvObject = this;
      AddRef();
      hr = S_OK;
    }
    else
    {
      hr = E_NOINTERFACE;
    }
  }
  return hr;
}

ULONG STDMETHODCALLTYPE CNotificationHandler::AddRef()
{
  return ++mdwRefCount;
}

ULONG STDMETHODCALLTYPE CNotificationHandler::Release()
{
  if (mdwRefCount)
  {
    --mdwRefCount;
  }
  return mdwRefCount;
}
