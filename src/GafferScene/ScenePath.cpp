//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2012, John Haddon. All rights reserved.
//  Copyright (c) 2013-2014, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "boost/bind.hpp"
#include "boost/algorithm/string/predicate.hpp"

#include "Gaffer/Context.h"
#include "Gaffer/PathFilter.h"
#include "Gaffer/Node.h"

#include "GafferScene/ScenePlug.h"
#include "GafferScene/ScenePath.h"
#include "GafferScene/SceneAlgo.h"

using namespace IECore;
using namespace Gaffer;
using namespace GafferScene;

IE_CORE_DEFINERUNTIMETYPED( ScenePath );

ScenePath::ScenePath( ScenePlugPtr scene, Gaffer::ContextPtr context, Gaffer::PathFilterPtr filter )
	:	Path( filter ), m_node( scene->node() ), m_scene( scene ), m_context( context )
{
}

ScenePath::ScenePath( ScenePlugPtr scene, Gaffer::ContextPtr context, const std::string &path, Gaffer::PathFilterPtr filter )
	:	Path( path, filter ), m_node( scene->node() ), m_scene( scene ), m_context( context )
{
}

ScenePath::ScenePath( ScenePlugPtr scene, Gaffer::ContextPtr context, const Names &names, const IECore::InternedString &root, Gaffer::PathFilterPtr filter )
	:	Path( names, root, filter ), m_node( scene->node() ), m_scene( scene ), m_context( context )
{
}

ScenePath::~ScenePath()
{
	if( havePathChangedSignal() )
	{
		m_context->changedSignal().disconnect( boost::bind( &ScenePath::contextChanged, this, ::_2 ) );
		if( m_node )
		{
			m_node->plugDirtiedSignal().disconnect( boost::bind( &ScenePath::plugDirtied, this, ::_1 ) );
		}
	}
}

void ScenePath::setContext( Gaffer::ContextPtr context )
{
	if( m_context == context )
	{
		return;
	}

	if( havePathChangedSignal() )
	{
		m_context->changedSignal().disconnect( boost::bind( &ScenePath::contextChanged, this, ::_2 ) );
		context->changedSignal().connect( boost::bind( &ScenePath::contextChanged, this, ::_2 ) );
	}

	m_context = context;
	emitPathChanged();
}

Gaffer::Context *ScenePath::getContext()
{
	return m_context.get();
}

const Gaffer::Context *ScenePath::getContext() const
{
	return m_context.get();
}

bool ScenePath::isValid() const
{
	if( !Path::isValid() )
	{
		return false;
	}

	Context::Scope scopedContext( m_context.get() );
	return exists( m_scene.get(), names() );
}

bool ScenePath::isLeaf() const
{
	// Any part of the scene could get children at any time
	return false;
}

PathPtr ScenePath::copy() const
{
	return new ScenePath( m_scene, m_context, names(), root(), const_cast<PathFilter *>( getFilter() ) );
}

void ScenePath::doChildren( std::vector<PathPtr> &children ) const
{
	Context::Scope scopedContext( m_context.get() );
	ConstInternedStringVectorDataPtr childNamesData = m_scene->childNames( names() );
	const std::vector<InternedString> &childNames = childNamesData->readable();
	ScenePlug::ScenePath childPath( names() );
	childPath.push_back( InternedString() ); // for the child name
	children.reserve( childNames.size() );
	for( std::vector<InternedString>::const_iterator it = childNames.begin(), eIt = childNames.end(); it != eIt; ++it )
	{
		childPath.back() = *it;
		children.push_back( new ScenePath( m_scene, m_context, childPath, root(), const_cast<PathFilter *>( getFilter() ) ) );
	}
}

void ScenePath::pathChangedSignalCreated()
{
	Path::pathChangedSignalCreated();
	if( m_node )
	{
		m_node->plugDirtiedSignal().connect( boost::bind( &ScenePath::plugDirtied, this, ::_1 ) );
	}
	m_context->changedSignal().connect( boost::bind( &ScenePath::contextChanged, this, ::_2 ) );
}

void ScenePath::contextChanged( const IECore::InternedString &key )
{
	if( !boost::starts_with( key.c_str(), "ui:" ) )
	{
		emitPathChanged();
	}
}

void ScenePath::plugDirtied( Gaffer::Plug *plug )
{
	if( plug == m_scene->childNamesPlug() )
	{
		emitPathChanged();
	}
}
