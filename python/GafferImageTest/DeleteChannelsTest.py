##########################################################################
#
#  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#      * Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      * Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided with
#        the distribution.
#
#      * Neither the name of John Haddon nor the names of
#        any other contributors to this software may be used to endorse or
#        promote products derived from this software without specific prior
#        written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

import os
import unittest

import IECore
import Gaffer
import GafferTest
import GafferImage

class DeleteChannelsTest( unittest.TestCase ) :

	checkerFile = os.path.expandvars( "$GAFFER_ROOT/python/GafferTest/images/checker.exr" )

	def testDirtyPropagation( self ) :

		r = GafferImage.ImageReader()
		r["fileName"].setValue( self.checkerFile )

		c = GafferImage.DeleteChannels()
		c["in"].setInput( r["out"] )

		cs = GafferTest.CapturingSlot( c.plugDirtiedSignal() )

		self.assertEqual( c["mode"].getValue(), GafferImage.DeleteChannels.Mode.Delete )
		c["mode"].setValue( GafferImage.DeleteChannels.Mode.Keep )

		dirtiedPlugs = set( [ x[0].relativeName( x[0].node() ) for x in cs ] )

		self.assertEqual( len(dirtiedPlugs), 3 )
		self.assertTrue( "mode" in dirtiedPlugs )
		self.assertTrue( "out" in dirtiedPlugs )
		self.assertTrue( "out.channelNames" in dirtiedPlugs )

		cs = GafferTest.CapturingSlot( c.plugDirtiedSignal() )

		c["channels"].setValue( IECore.StringVectorData( ["R"] ) )

		dirtiedPlugs = set( [ x[0].relativeName( x[0].node() ) for x in cs ] )
		self.assertEqual( len(dirtiedPlugs), 3 )
		self.assertTrue( "channels" in dirtiedPlugs )
		self.assertTrue( "out" in dirtiedPlugs )
		self.assertTrue( "out.channelNames" in dirtiedPlugs )

	def testDeleteChannels( self ) :

		r = GafferImage.ImageReader()
		r["fileName"].setValue( self.checkerFile )

		c = GafferImage.DeleteChannels()
		c["in"].setInput( r["out"] )
		c["mode"].setValue( GafferImage.DeleteChannels.Mode.Delete ) # Remove selected channels
		c["channels"].setValue( IECore.StringVectorData( [ "G", "A" ] ) )

		self.assertEqual( c["out"]["channelNames"].getValue(), IECore.StringVectorData( [ "R", "B" ] ) )

	def testKeepChannels( self ) :

		r = GafferImage.ImageReader()
		r["fileName"].setValue( self.checkerFile )

		c = GafferImage.DeleteChannels()
		c["in"].setInput( r["out"] )
		c["mode"].setValue( GafferImage.DeleteChannels.Mode.Keep ) # Keep selected channels
		c["channels"].setValue( IECore.StringVectorData( [ "G", "A" ] ) )

		self.assertEqual( c["out"]["channelNames"].getValue(), IECore.StringVectorData( [ "G", "A" ] ) )

	def testChannelDataHash( self ) :

		r = GafferImage.ImageReader()
		r["fileName"].setValue( self.checkerFile )

		d = GafferImage.DeleteChannels()
		d["in"].setInput( r["out"] )
		d["mode"].setValue( GafferImage.DeleteChannels.Mode.Delete )
		d["channels"].setValue( IECore.StringVectorData( [ "R" ] ) )

		# the channels that are passed through should have identical hashes to the input,
		# so they can share cache entries.
		self.assertEqual( r["out"].channelDataHash( "G", IECore.V2i( 0 ) ), d["out"].channelDataHash( "G", IECore.V2i( 0 ) ) )
		self.assertEqual( r["out"].channelDataHash( "B", IECore.V2i( 0 ) ), d["out"].channelDataHash( "B", IECore.V2i( 0 ) ) )
		self.assertEqual( r["out"].channelDataHash( "A", IECore.V2i( 0 ) ), d["out"].channelDataHash( "A", IECore.V2i( 0 ) ) )

	def testHashChanged( self ) :

		r = GafferImage.ImageReader()
		r["fileName"].setValue( self.checkerFile )

		c = GafferImage.DeleteChannels()
		c["in"].setInput( r["out"] )
		c["mode"].setValue( GafferImage.DeleteChannels.Mode.Keep ) # Keep selected channels
		c["channels"].setValue( r["out"]["channelNames"].getValue() )
		h = c["out"]["channelNames"].hash()

		c["channels"].setValue( IECore.StringVectorData( [ "R", "B" ] ) )
		h2 = c["out"]["channelNames"].hash()
		self.assertNotEqual( h, h2 )

	def testModePlug( self ) :

		n = GafferImage.DeleteChannels()
		self.assertEqual( n["mode"].defaultValue(), n.Mode.Delete )
		self.assertEqual( n["mode"].getValue(), n.Mode.Delete )

		n["mode"].setValue( n.Mode.Keep )
		self.assertEqual( n["mode"].getValue(), n.Mode.Keep )

		self.assertEqual( n["mode"].minValue(), n.Mode.Delete )
		self.assertEqual( n["mode"].maxValue(), n.Mode.Keep )

	def testImage( self ) :

		r = GafferImage.ImageReader()
		r["fileName"].setValue( self.checkerFile )

		d = GafferImage.DeleteChannels()
		d["in"].setInput( r["out"] )
		d["mode"].setValue( d.Mode.Keep )
		d["channels"].setValue( IECore.StringVectorData( [ "R" ] ) )

		ri = r["out"].image()
		di = d["out"].image()

		self.assertEqual( set( ri.keys() ), set( [ "R", "G", "B", "A" ] ) )
		self.assertEqual( di.keys(), [ "R" ] )
		self.assertEqual( di["R"].data, ri["R"].data )
