//
// Copyright (c) 2015, Microsoft Corporation
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

#include "pch.h"
#include "XmlUtil.h"

using namespace boost::property_tree;
using namespace boost::property_tree::xml_parser;

// Skeleton impllementation for reading & writing XML
// Not used at this time

XmlUtil::XmlUtil()
{
}

XmlUtil::~XmlUtil()
{
}

bool XmlUtil::LoadFromFile(const std::string filename)
{
	return true;
}

bool XmlUtil::LoadFromString(const std::string xml)
{
	return true;
}


std::string XmlUtil::GetNodeValue(const std::string path)
{
	return std::string();
}

std::string XmlUtil::GetNodeAttribute(std::string path, std::string attrName)
{
	return std::string();
}

