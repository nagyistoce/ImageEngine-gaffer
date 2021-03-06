##########################################################################
#
#  Copyright (c) 2015, Image Engine Design Inc. All rights reserved.
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

import Gaffer
import GafferUI
import GafferImage

Gaffer.Metadata.registerNode(

	GafferImage.Merge,

	"description",
	"""
	Composites two or more images together. The following operations
	are available :

	  - Add : A + B
	  - Atop : Ab + B(1-a)
	  - Divide : A / B
	  - In : Ab
	  - Out : A(1-b)
	  - Mask : Ba
	  - Matte : Aa + B(1.-a)
	  - Multiply : AB
	  - Over : A + B(1-a)
	  - Subtract : A - B
	  - Under : A(1-b) + B
	""",

	plugs = {

		"in" : [

			"description",
			"""
			The B input.
			""",

		],

		"in1" : [

			"description",
			"""
			The A input.
			""",

		],

		"operation" : [

			"description",
			"""
			The compositing operation used to merge the
			image together. See node documentation for
			more details.
			""",

			"preset:Add", 0,
			"preset:Atop", 1 ,
			"preset:Divide", 2,
			"preset:In", 3,
			"preset:Out", 4,
			"preset:Mask", 5,
			"preset:Matte", 6,
			"preset:Multiply", 7,
			"preset:Over", 8,
			"preset:Subtract", 9,
			"preset:Under", 10,

		],

	}

)

GafferUI.PlugValueWidget.registerCreator( GafferImage.Merge, "operation", GafferUI.PresetsPlugValueWidget )
