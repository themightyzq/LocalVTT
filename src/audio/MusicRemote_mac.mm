#ifdef __APPLE__

#include "audio/MusicRemote.h"
#include "utils/DebugConsole.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

// Helper: run an AppleScript string and return the result (or empty string on error)
static QString runAppleScript(const QString& script)
{
    @autoreleasepool {
        NSString* nsScript = script.toNSString();
        NSAppleScript* appleScript = [[NSAppleScript alloc] initWithSource:nsScript];
        NSDictionary* errorInfo = nil;
        NSAppleEventDescriptor* result = [appleScript executeAndReturnError:&errorInfo];

        if (result) {
            NSString* str = [result stringValue];
            if (str) {
                return QString::fromNSString(str);
            }
        }
        return QString();
    }
}

// Helper: detect which music app is running
enum class MusicApp { None, Spotify, AppleMusic };

static MusicApp detectMusicApp()
{
    @autoreleasepool {
        NSArray* apps = [[NSWorkspace sharedWorkspace] runningApplications];
        bool hasSpotify = false;
        bool hasMusic = false;

        for (NSRunningApplication* app in apps) {
            NSString* bundleId = app.bundleIdentifier;
            if ([bundleId isEqualToString:@"com.spotify.client"]) {
                hasSpotify = true;
            } else if ([bundleId isEqualToString:@"com.apple.Music"]) {
                hasMusic = true;
            }
        }

        // Prefer Spotify if both are running
        if (hasSpotify) return MusicApp::Spotify;
        if (hasMusic) return MusicApp::AppleMusic;
        return MusicApp::None;
    }
}

MusicRemote::NowPlaying MusicRemote::platformGetNowPlaying() const
{
    NowPlaying info;
    MusicApp app = detectMusicApp();

    if (app == MusicApp::Spotify) {
        QString state = runAppleScript(
            "tell application \"Spotify\"\n"
            "  if it is running then\n"
            "    return (get player state) as text\n"
            "  end if\n"
            "end tell");

        info.isPlaying = (state == "playing");

        if (!state.isEmpty()) {
            info.title = runAppleScript(
                "tell application \"Spotify\" to return name of current track");
            info.artist = runAppleScript(
                "tell application \"Spotify\" to return artist of current track");
            info.album = runAppleScript(
                "tell application \"Spotify\" to return album of current track");

            QString durStr = runAppleScript(
                "tell application \"Spotify\" to return (duration of current track) / 1000");
            info.duration = durStr.toDouble();

            QString posStr = runAppleScript(
                "tell application \"Spotify\" to return player position");
            info.position = posStr.toDouble();
        }
    } else if (app == MusicApp::AppleMusic) {
        QString state = runAppleScript(
            "tell application \"Music\"\n"
            "  if it is running then\n"
            "    return (get player state) as text\n"
            "  end if\n"
            "end tell");

        info.isPlaying = (state == "playing");

        if (!state.isEmpty()) {
            info.title = runAppleScript(
                "tell application \"Music\" to return name of current track");
            info.artist = runAppleScript(
                "tell application \"Music\" to return artist of current track");
            info.album = runAppleScript(
                "tell application \"Music\" to return album of current track");

            QString durStr = runAppleScript(
                "tell application \"Music\" to return duration of current track");
            info.duration = durStr.toDouble();

            QString posStr = runAppleScript(
                "tell application \"Music\" to return player position");
            info.position = posStr.toDouble();
        }
    }

    return info;
}

void MusicRemote::platformPlayPause()
{
    MusicApp app = detectMusicApp();
    if (app == MusicApp::Spotify) {
        runAppleScript("tell application \"Spotify\" to playpause");
    } else if (app == MusicApp::AppleMusic) {
        runAppleScript("tell application \"Music\" to playpause");
    }
}

void MusicRemote::platformNextTrack()
{
    MusicApp app = detectMusicApp();
    if (app == MusicApp::Spotify) {
        runAppleScript("tell application \"Spotify\" to next track");
    } else if (app == MusicApp::AppleMusic) {
        runAppleScript("tell application \"Music\" to next track");
    }
}

void MusicRemote::platformPreviousTrack()
{
    MusicApp app = detectMusicApp();
    if (app == MusicApp::Spotify) {
        runAppleScript("tell application \"Spotify\" to previous track");
    } else if (app == MusicApp::AppleMusic) {
        runAppleScript("tell application \"Music\" to back track");
    }
}

void MusicRemote::platformSetVolume(qreal volume)
{
    int vol = static_cast<int>(volume * 100);
    MusicApp app = detectMusicApp();
    if (app == MusicApp::Spotify) {
        runAppleScript(QString("tell application \"Spotify\" to set sound volume to %1").arg(vol));
    } else if (app == MusicApp::AppleMusic) {
        runAppleScript(QString("tell application \"Music\" to set sound volume to %1").arg(vol));
    }
}

#endif // __APPLE__
