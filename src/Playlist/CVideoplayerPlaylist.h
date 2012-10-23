/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#pragma once

#include <IVideoplayerSystem.h>
#include <Playlist/IVideoplayerPlaylist.h>
#include <IXml.h>

namespace VideoplayerPlugin
{

    class CVideoplayerPlaylist;
    struct SVideoInput :
        private IVideoplayerEventListener
    {
        SVideoInput();
        ~SVideoInput();

        bool init( XmlNodeRef xmlScene, CVideoplayerPlaylist* pPlaylist );
        void reset();

        string sClass;
        string sVideo;
        string sSound;

        float fStartAt;
        float fEndAfter;
        float fSpeed;

        eTimeSource eTS;
        eDropMode eDM;

        bool bLoop;
        bool bBlockGame;
        bool bSkippable;

        int nCustomWidth;
        int nCustomHeight;

        IVideoplayer* pVideo;

        // To create PlaylistEvents
        CVideoplayerPlaylist* pPlaylist;
        void OnStart();
        void OnFrame() {};
        void OnSeek() {};
        void OnEnd();
    };

    struct SSceneInput
    {
        ~SSceneInput();

        bool init( XmlNodeRef xmlScene, CVideoplayerPlaylist* pPlaylist );
        void reset();

        SVideoInput input;

        std::vector<S2DVideo*> v2DOutputs;
    };

    struct SScene
    {
        ~SScene();
        bool bSkippable;
        bool bLoop;
        float fDuration;
        std::vector<SSceneInput> vInputs;

        bool init( XmlNodeRef xmlScene, CVideoplayerPlaylist* pPlaylist );
        void reset();

        void Skip( bool bForce = false );
        bool IsPlaying();
        bool IsActive();

        void Resume();
        void Pause();
        bool Seek( float fPos );
    };

    struct SVideoEvent
    {
        // Needed because we need to decouple the VideoEventListeners from directly deleting videosthe dispatcher
        enum eVideoEventType
        {
            OnStart = 0,
            OnEnd,
        };

        eVideoEventType type;
        IVideoplayer*   pVideo;

        SVideoEvent( eVideoEventType type, IVideoplayer* pVideo )
        {
            this->type = type;
            this->pVideo = pVideo;
        };
    };

    class CVideoplayerPlaylist :
        public IVideoplayerPlaylist
    {
        private:
            XmlNodeRef  m_xmlPlaylist; // XML Nodes
            int         m_iScene;
            int         m_iSceneCount;
            bool        readNextScene();
            SScene      m_CurrentScene;
            std::vector<IVideoplayerPlaylistEventListener*>     vecQueue;

            bool        m_bLoop;
            bool        m_bSkippable;
            bool        m_bBlockGame;
            string      m_sFile;
            bool        m_bStart;
            bool        m_bSceneStart;
            bool        m_bPaused;

            std::queue<SVideoEvent> m_qVideoEvents;
        public:
            CVideoplayerPlaylist();
            ~CVideoplayerPlaylist();

            void QueueEvent( SVideoEvent::eVideoEventType type, IVideoplayer*    pVideo );

            virtual void OnStartPlaylist();
            virtual void OnBeginScene( int nIndex );
            virtual void OnVideoStart( IVideoplayer* pVideo );
            virtual void OnVideoEnd( IVideoplayer* pVideo ) ;
            virtual void OnEndScene( int nIndex );
            virtual void OnEndPlaylist();

            virtual bool Open( const char* sPlaylist, bool bLoop = false, bool bSkippable = true, bool bBlockGame = false, int nStartAtScene = -1, int nEndAtScene = -1 );
            virtual void Close();
            virtual void Advance( float deltaTime );

            virtual bool Seek( const int scene, float fPos );
            virtual int GetSceneCount();

            virtual IVideoplayer* GetSceneVideoplayer( int nIndex );
            virtual int GetSceneVideoplayerCount();

            virtual void RegisterListener( IVideoplayerPlaylistEventListener* item );
            virtual void UnregisterListener( IVideoplayerPlaylistEventListener* item );

            virtual float GetStart()
            {
                return 0;
            };
            virtual float GetPosition()
            {
                return 0;
            };
            virtual float GetDuration()
            {
                return 0;
            };
            virtual float GetEnd()
            {
                return 0;
            };

            virtual void SetSpeed( float fSpeed = 1.0f )
            {
                ;
            };
            virtual void Skip( bool bForce = false );
            virtual bool ReOpen();
            virtual bool IsPlaying();
            virtual bool IsActive();

            virtual void Resume();
            virtual void Pause();
            virtual bool Seek( float fPos );
    };

}