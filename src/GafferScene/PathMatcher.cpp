//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2012, John Haddon. All rights reserved.
//  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
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

#include "Gaffer/StringAlgo.h"

#include "GafferScene/PathMatcher.h"

using namespace std;
using namespace GafferScene;

//////////////////////////////////////////////////////////////////////////
// Node implementation
//////////////////////////////////////////////////////////////////////////

struct PathMatcher::Node
{

	typedef std::multimap<IECore::InternedString, Node *, Gaffer::MatchPatternLess> ChildMap;
	typedef ChildMap::iterator ChildMapIterator;
	typedef ChildMap::value_type ChildMapValue;
	typedef ChildMap::const_iterator ConstChildMapIterator;
	typedef std::pair<ChildMapIterator, ChildMapIterator> ChildMapRange;
	typedef std::pair<ConstChildMapIterator, ConstChildMapIterator> ConstChildMapRange;

	Node()
		:	terminator( false ), ellipsis( 0 )
	{
	}

	Node( const Node &other )
		:	terminator( other.terminator ), ellipsis( other.ellipsis ? new Node( *(other.ellipsis) ) : 0 )
	{
		ChildMapIterator hint = children.begin();
		for( ConstChildMapIterator it = other.children.begin(), eIt = other.children.end(); it != eIt; it++ )
		{
			hint = children.insert( hint, ChildMapValue( it->first, new Node( *(it->second) ) ) );
		}
	}

	~Node()
	{
		clearChildren();
	}

	ChildMapIterator childIterator( const IECore::InternedString &name )
	{
		ChildMapRange range = children.equal_range( name );
		while( range.first != range.second )
		{
			if( range.first->first == name )
			{
				return range.first;
			}
			range.first++;
		}
		return children.end();
	}

	ConstChildMapIterator childIterator( const IECore::InternedString &name ) const
	{
		ConstChildMapRange range = children.equal_range( name );
		while( range.first != range.second )
		{
			if( range.first->first == name )
			{
				return range.first;
			}
			range.first++;
		}
		return children.end();
	}

	// returns the child exactly matching name.
	Node *child( const IECore::InternedString &name )
	{
		ChildMapIterator it = childIterator( name );
		if( it != children.end() )
		{
			return it->second;
		}
		return 0;
	}

	// returns the range of children which /may/ match name
	// when wildcards are taken into account.
	ChildMapRange childRange( const IECore::InternedString &name )
	{
		return children.equal_range( name );
	}

	ConstChildMapRange childRange( const IECore::InternedString &name ) const
	{
		return children.equal_range( name );
	}

	bool operator == ( const Node &other ) const
	{
		if( terminator != other.terminator )
		{
			return false;
		}

		if( children.size() != other.children.size() )
		{
			return false;
		}

		for( ConstChildMapIterator it = children.begin(), eIt = children.end(); it != eIt; it++ )
		{
			ConstChildMapIterator oIt = other.childIterator( it->first );
			if( oIt == other.children.end() )
			{
				return false;
			}
			if( !(*(it->second) == *(oIt->second) ) )
			{
				return false;
			}
		}

		if( (bool)ellipsis != (bool)other.ellipsis )
		{
			return false;
		}
		else if( ellipsis )
		{
			if( (*ellipsis) != (*other.ellipsis) )
			{
				return false;
			}
		}

		return true;
	}

	bool operator != ( const Node &other )
	{
		return !( *this == other );
	}

	bool clearChildren()
	{
		const bool result = !children.empty() || ellipsis;
		ChildMap::iterator it, eIt;
		for( it = children.begin(), eIt = children.end(); it != eIt; it++ )
		{
			delete it->second;
		}
		children.clear();
		delete ellipsis;
		ellipsis = NULL;
		return result;
	}

	bool isEmpty()
	{
		return !terminator && !ellipsis && children.empty();
	}

	bool terminator;
	// map of child nodes
	ChildMap children;
	// child node for "...". this is stored separately as it uses
	// a slightly different matching algorithm.
	Node *ellipsis;

};

//////////////////////////////////////////////////////////////////////////
// PathMatcher implementation
//////////////////////////////////////////////////////////////////////////

static IECore::InternedString g_ellipsis( "..." );

PathMatcher::PathMatcher()
{
	m_root = boost::shared_ptr<Node>( new Node );
}

PathMatcher::PathMatcher( const PathMatcher &other )
{
	m_root = boost::shared_ptr<Node>( new Node( *(other.m_root) ) );
}

void PathMatcher::clear()
{
	m_root = boost::shared_ptr<Node>( new Node );
}

bool PathMatcher::isEmpty() const
{
	return m_root->isEmpty();
}

void PathMatcher::paths( std::vector<std::string> &paths ) const
{
	pathsWalk( m_root.get(), "/", paths );
}

bool PathMatcher::operator == ( const PathMatcher &other ) const
{
	return *m_root == *other.m_root;
}

bool PathMatcher::operator != ( const PathMatcher &other ) const
{
	return !(*this == other );
}

unsigned PathMatcher::match( const std::string &path ) const
{
	std::vector<IECore::InternedString> tokenizedPath;
	Gaffer::tokenize( path, '/', tokenizedPath );
	return match( tokenizedPath );
}

unsigned PathMatcher::match( const std::vector<IECore::InternedString> &path ) const
{
   Node *node = m_root.get();
   if( !node )
   {
       return Filter::NoMatch;
   }

	unsigned result = Filter::NoMatch;
	matchWalk( node, path.begin(), path.end(), result );
	return result;
}

template<typename NameIterator>
void PathMatcher::matchWalk( Node *node, const NameIterator &start, const NameIterator &end, unsigned &result ) const
{
	// see if we've matched to the end of the path, and terminate the recursion if we have.
	if( start == end )
	{
		if( node->terminator )
		{
			result |= Filter::ExactMatch;
		}
		if( node->children.size() )
		{
			result |= Filter::DescendantMatch;
		}
		if( node->ellipsis )
		{
			result |= Filter::DescendantMatch;
			if( node->ellipsis->terminator )
			{
				result |= Filter::ExactMatch;
			}
		}
		return;
	}

	// we haven't matched to the end of the path - there are still path elements
	// to check. if this node is a terminator then we have found an ancestor match
	// though.
	if( node->terminator )
	{
		result |= Filter::AncestorMatch;
	}

	// now we can match the remainder of the path against child branches to see
	// if we have any exact or descendant matches.
	Node::ChildMapRange range = node->childRange( *start );
	if( range.first != range.second )
	{
		NameIterator newStart = start; newStart++;
		for( Node::ChildMapIterator it = range.first; it != range.second; it++ )
		{
			if( Gaffer::match( start->c_str(), it->first.c_str() ) )
			{
				matchWalk( it->second, newStart, end, result );
				// if we've found every kind of match then we can terminate early,
				// but otherwise we need to keep going even though we may
				// have found some of the match types already.
				if( result == Filter::EveryMatch )
				{
					return;
				}
			}
		}
	}

	if( node->ellipsis )
	{
		result |= Filter::DescendantMatch;
		if( node->ellipsis->terminator )
		{
			result |= Filter::ExactMatch;
		}

		NameIterator newStart = start;
		while( newStart != end )
		{
			matchWalk( node->ellipsis, newStart, end, result );
			if( result == Filter::EveryMatch )
			{
				return;
			}
			newStart++;
		}
	}
}

bool PathMatcher::addPath( const std::string &path )
{
	std::vector<IECore::InternedString> tokenizedPath;
	Gaffer::tokenize( path, '/', tokenizedPath );
	return addPath( tokenizedPath );
}

bool PathMatcher::addPath( const std::vector<IECore::InternedString> &path )
{
	return addPath( path.begin(), path.end() );
}

template<typename NameIterator>
bool PathMatcher::addPath( const NameIterator &start, const NameIterator &end )
{
	Node *node = m_root.get();
	for( NameIterator it = start; it != end; ++it )
	{
		Node *nextNode = 0;
		const IECore::InternedString name( *it );
		if( name == g_ellipsis )
		{
			nextNode = node->ellipsis;
			if( !nextNode )
			{
				nextNode = new Node;
				node->ellipsis = nextNode;
			}
		}
		else
		{
			nextNode = node->child( name );
			if( !nextNode )
			{
				nextNode = new Node;
				node->children.insert( Node::ChildMapValue( name, nextNode ) );
			}
		}
		node = nextNode;
	}

	bool result = !node->terminator;
	node->terminator = true;
	return result;
}

bool PathMatcher::removePath( const std::string &path )
{
	std::vector<IECore::InternedString> tokenizedPath;
	Gaffer::tokenize( path, '/', tokenizedPath );
	return removePath( tokenizedPath );
}

bool PathMatcher::removePath( const std::vector<IECore::InternedString> &path )
{
	bool result = false;
	removeWalk( m_root.get(), path.begin(), path.end(), /* prune = */ false, result );
	return result;
}

bool PathMatcher::addPaths( const PathMatcher &paths )
{
	return addPathsWalk( m_root.get(), paths.m_root.get() );
}

bool PathMatcher::removePaths( const PathMatcher &paths )
{
	return removePathsWalk( m_root.get(), paths.m_root.get() );
}

bool PathMatcher::prune( const std::string &path )
{
	std::vector<IECore::InternedString> tokenizedPath;
	Gaffer::tokenize( path, '/', tokenizedPath );
	return prune( tokenizedPath );;
}

bool PathMatcher::prune( const std::vector<IECore::InternedString> &path )
{
	bool result = false;
	removeWalk( m_root.get(), path.begin(), path.end(), /* prune = */ true, result );
	return result;
}

template<typename NameIterator>
void PathMatcher::removeWalk( Node *node, const NameIterator &start, const NameIterator &end, const bool prune, bool &removed )
{
	if( start == end )
	{
		// we've found the end of the path we wish to remove.
		if( prune )
		{
			removed = node->clearChildren();
		}
		removed = removed || node->terminator;
		node->terminator = false;
		return;
	}

	const IECore::InternedString name( *start );
	Node::ChildMapIterator childIt = node->children.end();
	Node *childNode = 0;
	if( name == g_ellipsis )
	{
		childNode = node->ellipsis;
	}
	else
	{
		childIt = node->childIterator( name );
		if( childIt != node->children.end() )
		{
			childNode = childIt->second;
		}
	}

	if( !childNode )
	{
		return;
	}

	NameIterator childStart = start; childStart++;
	removeWalk( childNode, childStart, end, prune, removed );
	if( childNode->isEmpty() )
	{
		delete childNode;
		if( childIt != node->children.end() )
		{
			node->children.erase( childIt );
		}
		else
		{
			node->ellipsis = 0;
		}
	}
}

bool PathMatcher::addPathsWalk( Node *node, const Node *srcNode )
{
	bool result = false;
	if( !node->terminator && srcNode->terminator )
	{
		node->terminator = result = true;
	}

	for( Node::ChildMap::const_iterator it = srcNode->children.begin(), eIt = srcNode->children.end(); it != eIt; ++it )
	{
		const Node *srcChild = it->second;
		if( Node *child = node->child( it->first ) )
		{
			// result must be on right of ||, to avoid short-circuiting addPathsWalk().
			result = addPathsWalk( child, srcChild ) || result;
		}
		else
		{
			node->children.insert( Node::ChildMapValue( it->first, new Node( *srcChild ) ) );
			result = true; // source node can only exist if it or a descendant is a terminator
		}
	}

	if( srcNode->ellipsis )
	{
		if( node->ellipsis )
		{
			// result must be on right of ||, to avoid short-circuiting addPathsWalk().
			result = addPathsWalk( node->ellipsis, srcNode->ellipsis ) || result;
		}
		else
		{
			node->ellipsis = new Node( *srcNode->ellipsis );
			result = true; // source node can only exist if it or a descendant is a terminator
		}
	}

	return result;
}

bool PathMatcher::removePathsWalk( Node *node, const Node *srcNode )
{
	bool result = false;
	if( node->terminator && srcNode->terminator )
	{
		node->terminator = false;
		result = true;
	}

	for( Node::ChildMap::const_iterator it = srcNode->children.begin(), eIt = srcNode->children.end(); it != eIt; ++it )
	{
		const Node::ChildMapIterator childIt = node->childIterator( it->first );
		if( childIt != node->children.end() )
		{
			Node *child = childIt->second;
			if( removePathsWalk( child, it->second ) )
			{
				result = true;
				if( child->isEmpty() )
				{
					node->children.erase( childIt );
					delete child;
				}
			}
		}
	}

	if( node->ellipsis && srcNode->ellipsis )
	{
		if( removePathsWalk( node->ellipsis, srcNode->ellipsis ) )
		{
			result = true;
			if( node->ellipsis->isEmpty() )
			{
				delete node->ellipsis;
				node->ellipsis = NULL;
			}
		}
	}

	return result;
}

void PathMatcher::pathsWalk( Node *node, const std::string &path, std::vector<std::string> &paths ) const
{
	if( node->terminator )
	{
		paths.push_back( path );
	}

	for( Node::ChildMapIterator it = node->children.begin(), eIt = node->children.end(); it != eIt; it++ )
	{
		std::string childPath = path;
		if( node != m_root.get() )
		{
			childPath += "/";
		}
		childPath += it->first;
		pathsWalk( it->second, childPath, paths );
	}

	if( node->ellipsis )
	{
		std::string childPath = path;
		if( node != m_root.get() )
		{
			childPath += "/";
		}
		childPath += "...";
		pathsWalk( node->ellipsis, childPath, paths );

	}
}
