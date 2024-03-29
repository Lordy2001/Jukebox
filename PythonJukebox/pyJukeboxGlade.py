#!/usr/bin/env python3

import os
import sys
import math

import glob
import mutagen

import serial
import threading
import time

import gi
gi.require_version('Gst', '1.0')
gi.require_version('Gtk','3.0')
from gi.repository import GLib, Gst, GObject, Gtk, Gio

GLADE_CSS   = "/home/pi/PythonJukebox/jukebox.css"

MUSIC_ROOT  = "/media/pi/MUSIC/"
COMM_PORT = '/dev/ttyUSB0'
SLEEP_MINUTES = 15
COVERSPIN_MINUTES = 5

class JukeboxMainWindow(Gtk.Window):
    def __init__(self):

        #Build the window
        self.builder = Gtk.Builder()
        self.builder.add_from_file("pyJukebox.glade")
        self.builder.connect_signals(self)
        self.builder.get_object("volSlider").set_inverted(True)
        self.window = self.builder.get_object("jukeboxWindow")

        #Apply CSS
        #cssFile = Gio.
        provider = Gtk.CssProvider()
        provider.load_from_path(GLADE_CSS)
        self.apply_css(self.window, provider)

        #Set up Gstreamer
        self.songQueue = []
        self.playstate = False

        Gst.init(None)
        self.player = Gst.ElementFactory.make("playbin", "player")
        fakesink = Gst.ElementFactory.make("fakesink", "fakesink")
        self.player.set_property("video-sink", fakesink)
        alsasink = Gst.ElementFactory.make("alsasink", "alsasink")
        self.player.set_property("audio-sink",alsasink)
        bus = self.player.get_bus()
        bus.add_signal_watch()
        bus.connect("message", self.on_message)
        self.setVol(50)

        #Hack to turn mouse cursor to a dot
        self.window.connect("realize", self.realize_cb)

        #Set up sleep mode bool and sleep timer
        self.sleepTimer = 0
        self.sleepMode = True
        self.coverSpinTimer = 0

        #Open Serial port
        try:
            self.serial = serial.Serial(COMM_PORT, timeout = 1)
        except expression as identifier:
            self.onDestroy()

        if(not self.serial.is_open):
            Gtk.main_quit()

        #Set up Serial command listener to listen for song selections from keypad
        self.alive = threading.Event()
        self.cmdListenerThread = threading.Thread(target = self.ComPortThread)
        self.cmdListenerThread.setDaemon(1)
        self.alive.set()
        self.cmdListenerThread.start()

        # Set up 1 minute timer to spin the album covers each minute
        self.sIdMinuteTimer = GLib.timeout_add_seconds(60, self.onMinuteTick)

        #Final prep and show the window
        self.updateDisp("  ")
        self.sleep(False)
        self.spinCover()
        self.window.show()


    def apply_css(self, widget, provider):
        Gtk.StyleContext.add_provider(widget.get_style_context(),
                                      provider,
                                      Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        if isinstance(widget, Gtk.Container):
            widget.forall(self.apply_css, provider)


    def onDestroy(self, *args):
        if self.coverSpinThread is not None:
            self.coverSpinThread.join()
            self.coverSpinThread = None
        if self.cmdListenerThread is not None:
            self.alive.clear()          # clear alive event for thread
            self.cmdListenerThread.join()          # wait until thread has finished
            self.cmdListenerThread = None
        Gtk.main_quit()

    def onCloseBtn(self, *args):
        if self.coverSpinThread is not None:
            self.coverSpinThread.join()
            self.coverSpinThread = None
        if self.cmdListenerThread is not None:
            self.alive.clear()          # clear alive event for thread
            self.cmdListenerThread.join()          # wait until thread has finished
            self.cmdListenerThread = None
        Gtk.main_quit()

    def onSleepBtn(self, *args):
        self.sleep(True)

    def onVolChange(self, *args):
        self.setVol(self.builder.get_object("volAdjuster").get_value())

    def onSkip(self, button):
        print("Skip")
        self.player.set_state(Gst.State.NULL)
        self.getNext()
    
    def onAddSong(self, *args):
        song = self.builder.get_object("addSongTxtBox").get_text()
        print("Adding Song %r"%song)
        self.addSong(song)

    def on_message(self, bus, message):
        t = message.type
        s = t.get_name(t)        
        print("Got Message: %s"%(s))
        if t == Gst.MessageType.EOS:
            self.player.set_state(Gst.State.NULL)
            self.playstate = False 
            self.getNext()
        elif t == Gst.MessageType.ERROR:
            self.player.set_state(Gst.State.NULL)
            self.playstate = False 
            self.getNext()
            err, debug = message.parse_error()
            print ("Error: %s" % err, debug)

    def onMinuteTick(self):
        # if(not self.sleepMode):
        #     self.sleepTimer += 1
        #     print("Sleep Timer:%r"%self.sleepTimer)
        #     if(self.sleepTimer >=  SLEEP_MINUTES):
        #         self.sleep(True)

        self.coverSpinTimer += 1
        if(self.coverSpinTimer >= COVERSPIN_MINUTES):
            self.coverSpinTimer = 0
            self.spinCover()
        return True     #Do this again in a minute

    def realize_cb(self, widget):
        pass
        # pixmap = Gtk.Gdk.Pixmap(None, 1, 1, 1)
        # color = Gtk.Gdk.Color()
        # cursor = Gtk.Gdk.Cursor(pixmap, pixmap, color, color, 0, 0)
        # widget.window.set_cursor(cursor)
        
    def ComPortThread(self):
        while self.alive.isSet():
            b = self.serial.read(1)
            if(b != None):
                b = b.decode("utf-8")
                if(b == '#'):
                    cmd = self.serial.read(3)
                    cmd = cmd.decode("utf-8")
                    print("Got: "+ b + cmd)
                    GLib.idle_add(self.addSong, cmd[0:2])

    def spinCoverThread(self):
        print("Spinning Cover")
        self.sendCmd(b'#X1\n')
        time.sleep(11)
        print("Done Spinning Cover")
        self.sendCmd(b'#X0\n')

    def spinCover(self):
        if(not self.sleepMode):
            self.coverSpinThread = threading.Thread(target = self.spinCoverThread)
            self.coverSpinThread.setDaemon(1)
            self.coverSpinThread.start()

    def start(self, song,sfile,title,artist):
        print("Playing: %r" % sfile)
        self.builder.get_object("txtSongTitle").set_label(title)
        self.builder.get_object("txtSongArtist").set_label(artist)
        self.player.set_property("uri", "file://" + sfile)
        self.player.set_state(Gst.State.PLAYING)
        self.playstate = True
        self.updateDisp(song)
        self.sleepTimer = 0

    def setVol(self, newVol):
        newVol = newVol/100
        newVol = math.exp(6.908 * newVol)/1000
        print("New Volume = %r"%(newVol))
        self.player.set_property("volume", newVol)

    def addSong(self, song):
        sfile = self.getFile(song.strip())
        sfile = os.path.realpath(sfile)
        if(sfile != -1):
            m = mutagen.File(sfile)
            title = m['title'][0]
            artist = m['artist'][0]
        if(self.playstate):
            self.songQueue.append((song,sfile,title,artist))
            self.updatePlaylist()
        else:
            self.start(song,sfile,title,artist)
        self.sleep(False)

    def getNext(self):
        if(len(self.songQueue)):
            (song,sfile,title,artist) = self.songQueue.pop(0)
            self.start(song,sfile,title,artist)
            self.updatePlaylist()
        else:
            self.builder.get_object("txtSongTitle").set_label("")
            self.builder.get_object("txtSongArtist").set_label("")
            self.updateDisp("  ")
            self.playstate = False

    def updatePlaylist(self):
        self.builder.get_object("txtSongCount").set_label("%r"%len(self.songQueue))
        buff = ""
        for (song,sfile,title,artist) in self.songQueue:
            buff += "%s\t%s, %s \n"%(song,title,artist)
        self.builder.get_object("txtBoxSongList").get_buffer().set_text(buff)

    def getFile(self, index):
        fnames = glob.glob(MUSIC_ROOT + index + "*")
        print("Finding %r"%index)
        if(len(fnames) > 0):
            return fnames[0]
        else:
            #TODO should throw an exception here
            return -1 

    def sleep(self, sleep):
        if(sleep != self.sleepMode):
            if(sleep):
                print("Goodnight")
                self.sleepMode = True;
                self.sendCmd(b'#Z0\n')
                self.songQueue.clear()
                self.updatePlaylist()
                self.player.set_state(Gst.State.NULL)
                self.getNext()
            else:
                self.sleepMode = False;
                self.sendCmd(b'#Z1\n')

    def updateDisp(self, song):
        cmd = bytearray("#" + song + '\n','utf-8')
        self.sendCmd(cmd)

    def sendCmd(self,cmd):
        if(self.serial.is_open):
            self.serial.write(cmd)


if __name__ == '__main__':
    app = JukeboxMainWindow()
    Gtk.main()
