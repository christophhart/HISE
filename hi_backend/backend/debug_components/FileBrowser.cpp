/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

void FileBrowserToolbarFactory::getAllToolbarItemIds(Array<int> &ids)
{
	ids.clear();

    ids.add(FileBrowser::ShowFavoritePopup);
	ids.add(FileBrowser::Back);
	ids.add(FileBrowser::Forward);
}

ToolbarItemComponent * FileBrowserToolbarFactory::createItem(int itemId)
{
	ToolbarButton *b = new ToolbarButton(itemId, "", FileBrowserToolbarFactory::FileBrowserToolbarPaths::createPath(itemId, false),
		FileBrowserToolbarPaths::createPath(itemId, true));

	b->setCommandToTrigger(browser->browserCommandManager, itemId, true);

	return b;
}

Drawable * FileBrowserToolbarFactory::FileBrowserToolbarPaths::createPath(int id, bool isOn)
{
	Path path;

	switch (id)
	{
	case FileBrowser::ShowFavoritePopup:
	{
		static const unsigned char pathData[] = { 110, 109, 126, 176, 125, 67, 198, 239, 29, 67, 98, 200, 216, 123, 67, 77, 166, 29, 67, 169, 105, 122, 67, 140, 12, 28, 67, 158, 219, 120, 67, 47, 17, 27, 67, 98, 29, 45, 118, 67, 89, 32, 25, 67, 177, 175, 115, 67, 31, 230, 22, 67, 4, 214, 112, 67, 86, 52, 21, 67, 98, 27, 228, 110, 67, 242, 88, 21, 67, 152, 112, 109, 67, 161,
			87, 23, 67, 87, 203, 107, 67, 117, 91, 24, 67, 98, 200, 34, 105, 67, 61, 81, 26, 67, 173, 180, 102, 67, 148, 178, 28, 67, 229, 180, 99, 67, 3, 32, 30, 67, 98, 209, 143, 98, 67, 167, 37, 29, 67, 246, 58, 100, 67, 82, 176, 26, 67, 146, 108, 100, 67, 173, 66, 25, 67, 98, 37, 126, 101, 67, 6, 228, 21, 67, 213, 241, 102, 67, 207,
			161, 18, 67, 123, 188, 103, 67, 109, 46, 15, 67, 98, 205, 255, 102, 67, 52, 96, 13, 67, 79, 167, 100, 67, 175, 156, 12, 67, 5, 46, 99, 67, 86, 92, 11, 67, 98, 127, 126, 96, 67, 38, 112, 9, 67, 189, 122, 93, 67, 201, 220, 7, 67, 239, 49, 91, 67, 129, 115, 5, 67, 98, 121, 197, 91, 67, 104, 15, 4, 67, 0, 160, 94, 67, 44, 227, 4,
			67, 20, 11, 96, 67, 93, 161, 4, 67, 98, 12, 148, 99, 67, 247, 154, 4, 67, 75, 32, 103, 67, 174, 250, 4, 67, 16, 167, 106, 67, 109, 170, 4, 67, 98, 91, 36, 108, 67, 37, 104, 3, 67, 191, 36, 108, 67, 160, 240, 0, 67, 210, 224, 108, 67, 158, 77, 254, 66, 98, 119, 224, 109, 67, 172, 1, 248, 66, 153, 113, 110, 67, 106, 76, 241,
			66, 243, 7, 112, 67, 141, 118, 235, 66, 98, 54, 136, 113, 67, 27, 179, 235, 66, 142, 160, 113, 67, 134, 163, 241, 66, 88, 79, 114, 67, 121, 45, 244, 66, 98, 18, 109, 115, 67, 224, 226, 250, 66, 182, 42, 116, 67, 207, 238, 0, 67, 0, 142, 117, 67, 151, 48, 4, 67, 98, 87, 54, 119, 67, 163, 55, 5, 67, 17, 143, 121, 67, 218, 116,
			4, 67, 153, 124, 123, 67, 64, 154, 4, 67, 98, 30, 202, 126, 67, 86, 148, 4, 67, 201, 19, 129, 67, 5, 21, 4, 67, 194, 181, 130, 67, 173, 176, 4, 67, 98, 188, 226, 130, 67, 126, 39, 6, 67, 254, 124, 129, 67, 144, 41, 7, 67, 119, 253, 128, 67, 55, 52, 8, 67, 98, 141, 34, 127, 67, 80, 77, 10, 67, 130, 11, 124, 67, 189, 21, 12, 67,
			83, 96, 121, 67, 75, 105, 14, 67, 98, 72, 233, 120, 67, 38, 78, 16, 67, 43, 92, 122, 67, 73, 77, 18, 67, 28, 209, 122, 67, 54, 46, 20, 67, 98, 6, 220, 123, 67, 134, 80, 23, 67, 77, 95, 125, 67, 122, 92, 26, 67, 151, 205, 125, 67, 154, 167, 29, 67, 108, 240, 195, 125, 67, 126, 212, 29, 67, 108, 123, 176, 125, 67, 197, 239,
			29, 67, 108, 123, 176, 125, 67, 197, 239, 29, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));
		break;
	}
	case FileBrowser::AddFavorite:
	{
		static const unsigned char pathData[] = { 110, 109, 72, 146, 248, 65, 24, 78, 12, 67, 98, 56, 245, 248, 65, 215, 210, 9, 67, 193, 193, 247, 65, 48, 84, 7, 67, 108, 64, 249, 65, 70, 219, 4, 67, 98, 10, 86, 252, 65, 48, 159, 2, 67, 59, 94, 7, 66, 242, 63, 1, 67, 76, 2, 16, 66, 75, 129, 1, 67, 98, 71, 202, 70, 66, 81, 130, 1, 67, 113, 146, 125, 66, 128, 127, 1, 67, 39, 45,
			154, 66, 147, 130, 1, 67, 98, 218, 18, 159, 66, 234, 141, 1, 67, 150, 186, 162, 66, 29, 249, 3, 67, 145, 36, 162, 66, 190, 84, 6, 67, 98, 114, 12, 162, 66, 143, 154, 8, 67, 213, 87, 162, 66, 183, 227, 10, 67, 8, 249, 161, 66, 51, 39, 13, 67, 98, 161, 51, 161, 66, 72, 99, 15, 67, 5, 154, 156, 66, 135, 194, 16, 67, 253, 71, 152,
			66, 46, 129, 16, 67, 98, 255, 199, 121, 66, 40, 128, 16, 67, 213, 255, 66, 66, 249, 130, 16, 67, 248, 55, 12, 66, 230, 127, 16, 67, 98, 102, 127, 3, 66, 99, 111, 16, 67, 145, 53, 248, 65, 220, 123, 14, 67, 70, 146, 248, 65, 23, 78, 12, 67, 99, 109, 253, 96, 202, 66, 66, 77, 247, 66, 98, 145, 177, 198, 66, 79, 186, 246, 66,
			82, 211, 195, 66, 205, 134, 243, 66, 59, 183, 192, 66, 22, 144, 241, 66, 98, 57, 90, 187, 66, 105, 174, 237, 66, 97, 95, 182, 66, 245, 57, 233, 66, 7, 172, 176, 66, 100, 214, 229, 66, 98, 55, 200, 172, 66, 157, 31, 230, 66, 49, 225, 169, 66, 250, 28, 234, 66, 174, 150, 166, 66, 164, 36, 236, 66, 98, 145, 69, 161, 66, 52,
			16, 240, 66, 90, 105, 156, 66, 226, 210, 244, 66, 202, 105, 150, 66, 190, 173, 247, 66, 98, 161, 31, 148, 66, 6, 185, 245, 66, 236, 117, 151, 66, 93, 206, 240, 66, 37, 217, 151, 66, 17, 243, 237, 66, 98, 75, 252, 153, 66, 197, 53, 231, 66, 170, 227, 156, 66, 85, 177, 224, 66, 247, 120, 158, 66, 146, 202, 217, 66, 98, 154,
			255, 156, 66, 30, 46, 214, 66, 159, 78, 152, 66, 20, 167, 212, 66, 11, 92, 149, 66, 99, 38, 210, 66, 98, 254, 252, 143, 66, 3, 78, 206, 66, 122, 245, 137, 66, 74, 39, 203, 66, 222, 99, 133, 66, 186, 84, 198, 66, 98, 243, 138, 134, 66, 135, 140, 195, 66, 255, 63, 140, 66, 15, 52, 197, 66, 41, 22, 143, 66, 114, 176, 196, 66,
			98, 24, 40, 150, 66, 170, 163, 196, 66, 151, 64, 157, 66, 20, 99, 197, 66, 31, 78, 164, 66, 149, 194, 196, 66, 98, 183, 72, 167, 66, 4, 62, 194, 66, 125, 73, 167, 66, 251, 78, 189, 66, 165, 193, 168, 66, 90, 187, 185, 66, 98, 238, 192, 170, 66, 105, 111, 179, 66, 50, 227, 171, 66, 38, 186, 172, 66, 232, 15, 175, 66, 73,
			228, 166, 66, 98, 110, 16, 178, 66, 216, 32, 167, 66, 31, 65, 178, 66, 67, 17, 173, 66, 176, 158, 179, 66, 54, 155, 175, 66, 98, 36, 218, 181, 66, 157, 80, 182, 66, 109, 85, 183, 66, 90, 75, 189, 66, 2, 28, 186, 66, 236, 206, 195, 66, 98, 174, 108, 189, 66, 2, 221, 197, 66, 36, 30, 194, 66, 114, 87, 196, 66, 49, 249, 197,
			66, 63, 162, 196, 66, 98, 61, 148, 204, 66, 99, 150, 196, 66, 37, 79, 211, 66, 201, 151, 195, 66, 9, 215, 217, 66, 22, 207, 196, 66, 98, 238, 138, 218, 66, 183, 188, 199, 66, 248, 243, 212, 66, 219, 192, 201, 66, 220, 245, 210, 66, 42, 214, 203, 66, 98, 25, 69, 205, 66, 91, 8, 208, 66, 3, 23, 199, 66, 53, 153, 211, 66, 165,
			192, 193, 66, 81, 64, 216, 66, 98, 144, 210, 192, 66, 5, 10, 220, 66, 84, 184, 195, 66, 75, 8, 224, 66, 53, 162, 196, 66, 39, 202, 227, 66, 98, 10, 184, 198, 66, 196, 14, 234, 66, 152, 190, 201, 66, 174, 38, 240, 66, 43, 155, 202, 66, 240, 188, 246, 66, 108, 222, 135, 202, 66, 182, 22, 247, 66, 108, 245, 96, 202, 66, 68,
			77, 247, 66, 99, 109, 242, 21, 83, 66, 189, 75, 223, 66, 108, 88, 124, 109, 66, 189, 75, 223, 66, 98, 253, 202, 118, 66, 189, 75, 223, 66, 37, 73, 126, 66, 209, 10, 227, 66, 37, 73, 126, 66, 35, 178, 231, 66, 108, 37, 73, 126, 66, 171, 114, 29, 67, 98, 37, 73, 126, 66, 84, 198, 31, 67, 253, 202, 118, 66, 222, 165, 33, 67,
			88, 124, 109, 66, 222, 165, 33, 67, 108, 242, 21, 83, 66, 222, 165, 33, 67, 98, 77, 199, 73, 66, 222, 165, 33, 67, 37, 73, 66, 66, 84, 198, 31, 67, 37, 73, 66, 66, 171, 114, 29, 67, 108, 37, 73, 66, 66, 35, 178, 231, 66, 98, 37, 73, 66, 66, 209, 10, 227, 66, 77, 199, 73, 66, 189, 75, 223, 66, 242, 21, 83, 66, 189, 75, 223,
			66, 99, 101, 0, 0 };
		path.loadPathFromData(pathData, sizeof(pathData));
		break;
	}
	case FileBrowser::RemoveFavorite:
	{
		static const unsigned char pathData[] = { 110, 109, 183, 109, 241, 66, 97, 96, 18, 67, 98, 177, 155, 241, 66, 195, 194, 15, 67, 164, 8, 241, 66, 27, 29, 13, 67, 123, 201, 241, 66, 238, 132, 10, 67, 98, 150, 59, 243, 66, 47, 235, 7, 67, 180, 40, 249, 66, 196, 67, 7, 67, 167, 219, 253, 66, 170, 147, 7, 67, 98, 167, 88, 12, 67, 115, 154, 7, 67, 148, 196, 25, 67, 209,
			133, 7, 67, 184, 46, 39, 67, 245, 157, 7, 67, 98, 105, 150, 41, 67, 85, 240, 7, 67, 182, 28, 43, 67, 107, 135, 10, 67, 36, 181, 42, 67, 40, 212, 12, 67, 98, 134, 165, 42, 67, 6, 24, 15, 67, 99, 228, 42, 67, 188, 99, 17, 67, 248, 136, 42, 67, 29, 162, 19, 67, 98, 235, 207, 41, 67, 221, 59, 22, 67, 92, 217, 38, 67, 72, 227, 22,
			67, 226, 127, 36, 67, 97, 147, 22, 67, 98, 11, 25, 23, 67, 231, 140, 22, 67, 63, 177, 9, 67, 157, 160, 22, 67, 4, 150, 248, 66, 134, 137, 22, 67, 98, 126, 152, 244, 66, 124, 69, 22, 67, 192, 82, 241, 66, 159, 101, 20, 67, 182, 109, 241, 66, 94, 96, 18, 67, 99, 109, 16, 213, 62, 67, 234, 184, 1, 67, 98, 90, 253, 60, 67, 113,
			111, 1, 67, 59, 142, 59, 67, 95, 171, 255, 66, 48, 0, 58, 67, 166, 180, 253, 66, 98, 175, 81, 55, 67, 251, 210, 249, 66, 67, 212, 52, 67, 133, 94, 245, 66, 150, 250, 49, 67, 245, 250, 241, 66, 98, 174, 8, 48, 67, 46, 68, 242, 66, 43, 149, 46, 67, 139, 65, 246, 66, 233, 239, 44, 67, 53, 73, 248, 66, 98, 91, 71, 42, 67, 197,
			52, 252, 66, 63, 217, 39, 67, 186, 123, 0, 67, 119, 217, 36, 67, 40, 233, 1, 67, 98, 98, 180, 35, 67, 204, 238, 0, 67, 136, 95, 37, 67, 239, 242, 252, 66, 36, 145, 37, 67, 163, 23, 250, 66, 98, 183, 162, 38, 67, 87, 90, 243, 66, 103, 22, 40, 67, 231, 213, 236, 66, 13, 225, 40, 67, 36, 239, 229, 66, 98, 95, 36, 40, 67, 176,
			82, 226, 66, 225, 203, 37, 67, 166, 203, 224, 66, 151, 82, 36, 67, 245, 74, 222, 66, 98, 17, 163, 33, 67, 149, 114, 218, 66, 79, 159, 30, 67, 220, 75, 215, 66, 129, 86, 28, 67, 76, 121, 210, 66, 98, 11, 234, 28, 67, 24, 177, 207, 66, 146, 196, 31, 67, 160, 88, 209, 66, 167, 47, 33, 67, 3, 213, 208, 66, 98, 158, 184, 36, 67,
			54, 200, 208, 66, 221, 68, 40, 67, 165, 135, 209, 66, 162, 203, 43, 67, 35, 231, 208, 66, 98, 238, 72, 45, 67, 146, 98, 206, 66, 81, 73, 45, 67, 138, 115, 201, 66, 101, 5, 46, 67, 231, 223, 197, 66, 98, 9, 5, 47, 67, 246, 147, 191, 66, 44, 150, 47, 67, 179, 222, 184, 66, 134, 44, 49, 67, 214, 8, 179, 66, 98, 201, 172, 50,
			67, 101, 69, 179, 66, 33, 197, 50, 67, 208, 53, 185, 66, 234, 115, 51, 67, 195, 191, 187, 66, 98, 164, 145, 52, 67, 42, 117, 194, 66, 73, 79, 53, 67, 231, 111, 201, 66, 147, 178, 54, 67, 121, 243, 207, 66, 98, 233, 90, 56, 67, 143, 1, 210, 66, 164, 179, 58, 67, 254, 123, 208, 66, 43, 161, 60, 67, 204, 198, 208, 66, 98, 177,
			238, 63, 67, 235, 186, 208, 66, 37, 76, 67, 67, 86, 188, 207, 66, 22, 144, 70, 67, 166, 243, 208, 66, 98, 9, 234, 70, 67, 70, 225, 211, 66, 142, 30, 68, 67, 105, 229, 213, 66, 128, 31, 67, 67, 185, 250, 215, 66, 98, 30, 71, 64, 67, 234, 44, 220, 66, 19, 48, 61, 67, 196, 189, 223, 66, 228, 132, 58, 67, 224, 100, 228, 66, 98,
			218, 13, 58, 67, 148, 46, 232, 66, 188, 128, 59, 67, 218, 44, 236, 66, 173, 245, 59, 67, 181, 238, 239, 66, 98, 151, 0, 61, 67, 83, 51, 246, 66, 222, 131, 62, 67, 60, 75, 252, 66, 40, 242, 62, 67, 191, 112, 1, 67, 108, 129, 232, 62, 67, 163, 157, 1, 67, 108, 12, 213, 62, 67, 234, 184, 1, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));
		break;
	}
	case FileBrowser::HardDisks:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 31, 1, 67, 0, 81, 59, 67, 98, 63, 197, 254, 66, 88, 73, 59, 67, 69, 30, 251, 66, 255, 119, 59, 67, 0, 45, 248, 66, 128, 92, 59, 67, 108, 0, 204, 48, 66, 128, 92, 59, 67, 98, 148, 10, 26, 66, 18, 233, 59, 67, 182, 37, 33, 66, 37, 90, 66, 67, 0, 0, 32, 66, 128, 70, 70, 67, 108, 0, 0, 32, 66, 192, 20, 149, 67, 98, 207,
			48, 34, 66, 244, 236, 151, 67, 33, 245, 59, 66, 135, 9, 151, 67, 0, 166, 75, 66, 64, 46, 151, 67, 108, 0, 205, 2, 67, 64, 46, 151, 67, 98, 104, 125, 8, 67, 38, 232, 150, 67, 142, 182, 6, 67, 156, 175, 147, 67, 0, 0, 7, 67, 128, 185, 145, 67, 108, 0, 0, 7, 67, 0, 144, 63, 67, 98, 134, 168, 6, 67, 137, 1, 60, 67, 161, 3, 4, 67, 195,
			93, 59, 67, 0, 31, 1, 67, 0, 81, 59, 67, 99, 109, 0, 144, 86, 66, 128, 90, 64, 67, 98, 154, 1, 114, 66, 97, 153, 64, 67, 248, 12, 127, 66, 180, 150, 74, 67, 0, 214, 103, 66, 0, 175, 78, 67, 98, 156, 67, 82, 66, 48, 122, 83, 67, 65, 25, 45, 66, 125, 197, 77, 67, 0, 98, 52, 66, 0, 204, 70, 67, 98, 170, 29, 55, 66, 66, 33, 67, 67,
			77, 240, 68, 66, 184, 77, 64, 67, 0, 220, 83, 66, 128, 92, 64, 67, 98, 30, 198, 84, 66, 52, 89, 64, 67, 94, 173, 85, 66, 121, 88, 64, 67, 0, 144, 86, 66, 128, 90, 64, 67, 99, 109, 0, 49, 246, 66, 0, 143, 64, 67, 98, 71, 134, 1, 67, 12, 202, 64, 67, 106, 148, 4, 67, 215, 37, 74, 67, 255, 71, 254, 66, 0, 252, 77, 67, 98, 148, 44,
			244, 66, 190, 121, 82, 67, 88, 195, 226, 66, 154, 33, 77, 67, 0, 45, 230, 66, 128, 152, 70, 67, 98, 3, 117, 231, 66, 201, 40, 67, 67, 53, 239, 237, 66, 132, 130, 64, 67, 0, 237, 244, 66, 128, 144, 64, 67, 98, 175, 90, 245, 66, 106, 141, 64, 67, 210, 198, 245, 66, 24, 141, 64, 67, 0, 49, 246, 66, 0, 143, 64, 67, 99, 109, 0,
			190, 179, 66, 0, 232, 71, 67, 98, 97, 68, 224, 66, 105, 157, 72, 67, 99, 119, 1, 67, 196, 39, 97, 67, 0, 4, 244, 66, 0, 111, 118, 67, 98, 222, 53, 232, 66, 204, 32, 134, 67, 5, 90, 175, 66, 50, 123, 139, 67, 0, 196, 137, 66, 0, 45, 133, 67, 98, 155, 243, 138, 66, 125, 169, 130, 67, 46, 31, 152, 66, 185, 15, 121, 67, 0, 198,
			132, 66, 128, 94, 125, 67, 98, 78, 107, 116, 66, 52, 242, 126, 67, 168, 203, 92, 66, 111, 175, 128, 67, 0, 146, 89, 66, 128, 52, 121, 67, 98, 3, 71, 51, 66, 7, 206, 99, 67, 239, 225, 118, 66, 162, 195, 72, 67, 0, 83, 170, 66, 128, 0, 72, 67, 108, 0, 103, 175, 66, 128, 233, 71, 67, 98, 154, 219, 176, 66, 108, 226, 71, 67, 79,
			78, 178, 66, 38, 226, 71, 67, 0, 190, 179, 66, 0, 232, 71, 67, 99, 109, 0, 128, 172, 66, 0, 174, 88, 67, 98, 64, 95, 147, 66, 59, 21, 89, 67, 124, 205, 128, 66, 94, 187, 103, 67, 0, 179, 139, 66, 128, 69, 115, 67, 98, 230, 242, 148, 66, 174, 157, 127, 67, 26, 214, 184, 66, 160, 126, 129, 67, 255, 103, 202, 66, 128, 39, 121,
			67, 98, 45, 142, 221, 66, 93, 22, 112, 67, 74, 196, 213, 66, 149, 63, 94, 67, 0, 210, 188, 66, 128, 252, 89, 67, 98, 132, 105, 184, 66, 215, 32, 89, 67, 185, 174, 179, 66, 57, 175, 88, 67, 0, 244, 174, 66, 128, 175, 88, 67, 98, 50, 33, 174, 66, 23, 171, 88, 67, 130, 79, 173, 66, 172, 170, 88, 67, 0, 128, 172, 66, 0, 174, 88,
			67, 99, 109, 0, 125, 246, 66, 0, 39, 141, 67, 98, 71, 172, 1, 67, 134, 68, 141, 67, 106, 186, 4, 67, 107, 242, 145, 67, 0, 148, 254, 66, 128, 221, 147, 67, 98, 149, 120, 244, 66, 95, 28, 150, 67, 88, 15, 227, 66, 77, 112, 147, 67, 0, 121, 230, 66, 192, 43, 144, 67, 98, 4, 193, 231, 66, 228, 115, 142, 67, 53, 59, 238, 66, 194,
			32, 141, 67, 0, 57, 245, 66, 192, 39, 141, 67, 98, 176, 166, 245, 66, 53, 38, 141, 67, 210, 18, 246, 66, 12, 38, 141, 67, 0, 125, 246, 66, 0, 39, 141, 67, 99, 109, 0, 148, 85, 66, 128, 52, 141, 67, 98, 27, 75, 111, 66, 6, 82, 141, 67, 167, 131, 123, 66, 235, 255, 145, 67, 0, 194, 101, 66, 0, 235, 147, 67, 98, 42, 139, 81, 66,
			223, 41, 150, 67, 174, 184, 46, 66, 205, 125, 147, 67, 0, 140, 53, 66, 64, 57, 144, 67, 98, 6, 28, 56, 66, 101, 129, 142, 67, 106, 16, 69, 66, 66, 46, 141, 67, 0, 12, 83, 66, 64, 53, 141, 67, 98, 95, 231, 83, 66, 181, 51, 141, 67, 164, 191, 84, 66, 140, 51, 141, 67, 0, 148, 85, 66, 128, 52, 141, 67, 99, 101, 0, 0 };
		path.loadPathFromData(pathData, sizeof(pathData));
	}
	case FileBrowser::Back:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 62, 67, 0, 239, 122, 67, 108, 0, 0, 62, 67, 0, 239, 122, 67, 108, 27, 0, 61, 67, 102, 245, 122, 67, 108, 218, 0, 60, 67, 148, 8, 123, 67, 108, 224, 2, 59, 67, 126, 40, 123, 67, 108, 208, 6, 58, 67, 15, 85, 123, 67, 108, 75, 13, 57, 67, 43, 142, 123, 67, 108, 240, 22, 56, 67, 173, 211, 123, 67, 108, 93, 36, 55,
			67, 105, 37, 124, 67, 108, 46, 54, 54, 67, 43, 131, 124, 67, 108, 250, 76, 53, 67, 182, 236, 124, 67, 108, 87, 105, 52, 67, 199, 97, 125, 67, 108, 215, 139, 51, 67, 19, 226, 125, 67, 108, 8, 181, 50, 67, 72, 109, 126, 67, 108, 114, 229, 49, 67, 13, 3, 127, 67, 108, 155, 29, 49, 67, 2, 163, 127, 67, 108, 2, 94, 48, 67, 96,
			38, 128, 67, 108, 35, 167, 47, 67, 238, 127, 128, 67, 108, 114, 249, 46, 67, 242, 221, 128, 67, 108, 94, 85, 46, 67, 46, 64, 129, 67, 108, 80, 187, 45, 67, 100, 166, 129, 67, 108, 171, 43, 45, 67, 84, 16, 130, 67, 108, 203, 166, 44, 67, 184, 125, 130, 67, 108, 5, 45, 44, 67, 75, 238, 130, 67, 108, 166, 190, 43, 67, 198,
			97, 131, 67, 108, 246, 91, 43, 67, 221, 215, 131, 67, 108, 51, 5, 43, 67, 70, 80, 132, 67, 108, 149, 186, 42, 67, 180, 202, 132, 67, 108, 76, 124, 42, 67, 216, 70, 133, 67, 108, 127, 74, 42, 67, 98, 196, 133, 67, 108, 79, 37, 42, 67, 3, 67, 134, 67, 108, 211, 12, 42, 67, 105, 194, 134, 67, 108, 27, 1, 42, 67, 68, 66, 135,
			67, 108, 0, 0, 42, 67, 128, 119, 135, 67, 108, 0, 0, 42, 67, 128, 119, 135, 67, 108, 0, 0, 42, 67, 128, 119, 135, 67, 108, 102, 6, 42, 67, 114, 247, 135, 67, 108, 148, 25, 42, 67, 19, 119, 136, 67, 108, 126, 57, 42, 67, 16, 246, 136, 67, 108, 15, 102, 42, 67, 24, 116, 137, 67, 108, 43, 159, 42, 67, 219, 240, 137, 67, 108,
			174, 228, 42, 67, 9, 108, 138, 67, 108, 106, 54, 43, 67, 82, 229, 138, 67, 108, 44, 148, 43, 67, 106, 92, 139, 67, 108, 183, 253, 43, 67, 4, 209, 139, 67, 108, 200, 114, 44, 67, 213, 66, 140, 67, 108, 20, 243, 44, 67, 149, 177, 140, 67, 108, 74, 126, 45, 67, 253, 28, 141, 67, 108, 15, 20, 46, 67, 200, 132, 141, 67, 108,
			4, 180, 46, 67, 180, 232, 141, 67, 108, 195, 93, 47, 67, 128, 72, 142, 67, 108, 223, 16, 48, 67, 240, 163, 142, 67, 108, 229, 204, 48, 67, 200, 250, 142, 67, 108, 94, 145, 49, 67, 210, 76, 143, 67, 108, 203, 93, 50, 67, 217, 153, 143, 67, 108, 170, 49, 51, 67, 171, 225, 143, 67, 108, 114, 12, 52, 67, 27, 36, 144, 67, 108,
			153, 237, 52, 67, 254, 96, 144, 67, 108, 142, 212, 53, 67, 46, 152, 144, 67, 108, 189, 192, 54, 67, 134, 201, 144, 67, 108, 143, 177, 55, 67, 231, 244, 144, 67, 108, 107, 166, 56, 67, 54, 26, 145, 67, 108, 178, 158, 57, 67, 90, 57, 145, 67, 108, 199, 153, 58, 67, 65, 82, 145, 67, 108, 9, 151, 59, 67, 216, 100, 145, 67,
			108, 214, 149, 60, 67, 22, 113, 145, 67, 108, 138, 149, 61, 67, 242, 118, 145, 67, 108, 0, 0, 62, 67, 128, 119, 145, 67, 108, 0, 0, 62, 67, 128, 119, 145, 67, 108, 0, 0, 62, 67, 128, 119, 145, 67, 108, 229, 255, 62, 67, 77, 116, 145, 67, 108, 37, 255, 63, 67, 182, 106, 145, 67, 108, 31, 253, 64, 67, 193, 90, 145, 67, 108,
			48, 249, 65, 67, 120, 68, 145, 67, 108, 181, 242, 66, 67, 234, 39, 145, 67, 108, 16, 233, 67, 67, 41, 5, 145, 67, 108, 162, 219, 68, 67, 75, 220, 144, 67, 108, 210, 201, 69, 67, 107, 173, 144, 67, 108, 5, 179, 70, 67, 165, 120, 144, 67, 108, 168, 150, 71, 67, 29, 62, 144, 67, 108, 40, 116, 72, 67, 247, 253, 143, 67, 108,
			248, 74, 73, 67, 92, 184, 143, 67, 108, 141, 26, 74, 67, 122, 109, 143, 67, 108, 100, 226, 74, 67, 127, 29, 143, 67, 108, 253, 161, 75, 67, 160, 200, 142, 67, 108, 220, 88, 76, 67, 18, 111, 142, 67, 108, 142, 6, 77, 67, 15, 17, 142, 67, 108, 161, 170, 77, 67, 211, 174, 141, 67, 108, 175, 68, 78, 67, 156, 72, 141, 67, 108,
			84, 212, 78, 67, 173, 222, 140, 67, 108, 52, 89, 79, 67, 73, 113, 140, 67, 108, 251, 210, 79, 67, 181, 0, 140, 67, 108, 89, 65, 80, 67, 59, 141, 139, 67, 108, 10, 164, 80, 67, 36, 23, 139, 67, 108, 205, 250, 80, 67, 186, 158, 138, 67, 108, 107, 69, 81, 67, 77, 36, 138, 67, 108, 180, 131, 81, 67, 41, 168, 137, 67, 108, 128,
			181, 81, 67, 159, 42, 137, 67, 108, 177, 218, 81, 67, 254, 171, 136, 67, 108, 44, 243, 81, 67, 151, 44, 136, 67, 108, 229, 254, 81, 67, 189, 172, 135, 67, 108, 0, 0, 82, 67, 128, 119, 135, 67, 108, 0, 0, 82, 67, 128, 119, 135, 67, 108, 0, 0, 82, 67, 128, 119, 135, 67, 108, 154, 249, 81, 67, 142, 247, 134, 67, 108, 108, 230,
			81, 67, 237, 119, 134, 67, 108, 130, 198, 81, 67, 240, 248, 133, 67, 108, 241, 153, 81, 67, 232, 122, 133, 67, 108, 213, 96, 81, 67, 38, 254, 132, 67, 108, 83, 27, 81, 67, 248, 130, 132, 67, 108, 151, 201, 80, 67, 175, 9, 132, 67, 108, 213, 107, 80, 67, 151, 146, 131, 67, 108, 74, 2, 80, 67, 253, 29, 131, 67, 108, 57, 141,
			79, 67, 44, 172, 130, 67, 108, 237, 12, 79, 67, 108, 61, 130, 67, 108, 184, 129, 78, 67, 4, 210, 129, 67, 108, 243, 235, 77, 67, 57, 106, 129, 67, 108, 254, 75, 77, 67, 78, 6, 129, 67, 108, 64, 162, 76, 67, 130, 166, 128, 67, 108, 36, 239, 75, 67, 18, 75, 128, 67, 108, 30, 51, 75, 67, 114, 232, 127, 67, 108, 165, 110, 74,
			67, 94, 68, 127, 67, 108, 56, 162, 73, 67, 80, 170, 126, 67, 108, 90, 206, 72, 67, 172, 26, 126, 67, 108, 145, 243, 71, 67, 203, 149, 125, 67, 108, 106, 18, 71, 67, 5, 28, 125, 67, 108, 117, 43, 70, 67, 166, 173, 124, 67, 108, 70, 63, 69, 67, 246, 74, 124, 67, 108, 116, 78, 68, 67, 51, 244, 123, 67, 108, 153, 89, 67, 67,
			149, 169, 123, 67, 108, 81, 97, 66, 67, 76, 107, 123, 67, 108, 60, 102, 65, 67, 127, 57, 123, 67, 108, 250, 104, 64, 67, 79, 20, 123, 67, 108, 45, 106, 63, 67, 212, 251, 122, 67, 108, 121, 106, 62, 67, 28, 240, 122, 67, 108, 0, 0, 62, 67, 0, 239, 122, 67, 108, 0, 0, 62, 67, 0, 239, 122, 67, 99, 109, 128, 219, 69, 67, 192,
			248, 128, 67, 108, 128, 219, 69, 67, 64, 246, 141, 67, 108, 128, 91, 47, 67, 128, 119, 135, 67, 108, 128, 219, 69, 67, 192, 248, 128, 67, 99, 101, 0, 0 };
		path.loadPathFromData(pathData, sizeof(pathData));
		break;
	}
	case FileBrowser::Forward:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 183, 107, 67, 0, 239, 122, 67, 108, 0, 183, 107, 67, 0, 239, 122, 67, 108, 27, 183, 106, 67, 102, 245, 122, 67, 108, 218, 183, 105, 67, 148, 8, 123, 67, 108, 224, 185, 104, 67, 126, 40, 123, 67, 108, 208, 189, 103, 67, 15, 85, 123, 67, 108, 75, 196, 102, 67, 43, 142, 123, 67, 108, 240, 205, 101, 67, 173, 211,
			123, 67, 108, 93, 219, 100, 67, 105, 37, 124, 67, 108, 46, 237, 99, 67, 43, 131, 124, 67, 108, 250, 3, 99, 67, 182, 236, 124, 67, 108, 87, 32, 98, 67, 199, 97, 125, 67, 108, 215, 66, 97, 67, 19, 226, 125, 67, 108, 8, 108, 96, 67, 72, 109, 126, 67, 108, 114, 156, 95, 67, 13, 3, 127, 67, 108, 155, 212, 94, 67, 2, 163, 127,
			67, 108, 2, 21, 94, 67, 96, 38, 128, 67, 108, 35, 94, 93, 67, 238, 127, 128, 67, 108, 114, 176, 92, 67, 242, 221, 128, 67, 108, 94, 12, 92, 67, 46, 64, 129, 67, 108, 80, 114, 91, 67, 100, 166, 129, 67, 108, 171, 226, 90, 67, 84, 16, 130, 67, 108, 203, 93, 90, 67, 184, 125, 130, 67, 108, 5, 228, 89, 67, 75, 238, 130, 67, 108,
			166, 117, 89, 67, 198, 97, 131, 67, 108, 246, 18, 89, 67, 221, 215, 131, 67, 108, 51, 188, 88, 67, 70, 80, 132, 67, 108, 149, 113, 88, 67, 180, 202, 132, 67, 108, 76, 51, 88, 67, 216, 70, 133, 67, 108, 127, 1, 88, 67, 98, 196, 133, 67, 108, 79, 220, 87, 67, 3, 67, 134, 67, 108, 211, 195, 87, 67, 105, 194, 134, 67, 108, 27,
			184, 87, 67, 68, 66, 135, 67, 108, 0, 183, 87, 67, 128, 119, 135, 67, 108, 0, 183, 87, 67, 128, 119, 135, 67, 108, 0, 183, 87, 67, 128, 119, 135, 67, 108, 102, 189, 87, 67, 114, 247, 135, 67, 108, 148, 208, 87, 67, 19, 119, 136, 67, 108, 126, 240, 87, 67, 16, 246, 136, 67, 108, 15, 29, 88, 67, 24, 116, 137, 67, 108, 43,
			86, 88, 67, 219, 240, 137, 67, 108, 174, 155, 88, 67, 9, 108, 138, 67, 108, 106, 237, 88, 67, 82, 229, 138, 67, 108, 44, 75, 89, 67, 106, 92, 139, 67, 108, 183, 180, 89, 67, 4, 209, 139, 67, 108, 200, 41, 90, 67, 213, 66, 140, 67, 108, 20, 170, 90, 67, 149, 177, 140, 67, 108, 74, 53, 91, 67, 253, 28, 141, 67, 108, 15, 203,
			91, 67, 200, 132, 141, 67, 108, 4, 107, 92, 67, 180, 232, 141, 67, 108, 195, 20, 93, 67, 128, 72, 142, 67, 108, 223, 199, 93, 67, 240, 163, 142, 67, 108, 229, 131, 94, 67, 200, 250, 142, 67, 108, 94, 72, 95, 67, 210, 76, 143, 67, 108, 203, 20, 96, 67, 217, 153, 143, 67, 108, 170, 232, 96, 67, 171, 225, 143, 67, 108, 114,
			195, 97, 67, 27, 36, 144, 67, 108, 153, 164, 98, 67, 254, 96, 144, 67, 108, 142, 139, 99, 67, 46, 152, 144, 67, 108, 189, 119, 100, 67, 134, 201, 144, 67, 108, 143, 104, 101, 67, 231, 244, 144, 67, 108, 107, 93, 102, 67, 54, 26, 145, 67, 108, 178, 85, 103, 67, 90, 57, 145, 67, 108, 199, 80, 104, 67, 65, 82, 145, 67, 108,
			9, 78, 105, 67, 216, 100, 145, 67, 108, 214, 76, 106, 67, 22, 113, 145, 67, 108, 138, 76, 107, 67, 242, 118, 145, 67, 108, 0, 183, 107, 67, 128, 119, 145, 67, 108, 0, 183, 107, 67, 128, 119, 145, 67, 108, 0, 183, 107, 67, 128, 119, 145, 67, 108, 229, 182, 108, 67, 77, 116, 145, 67, 108, 37, 182, 109, 67, 182, 106, 145,
			67, 108, 31, 180, 110, 67, 193, 90, 145, 67, 108, 48, 176, 111, 67, 120, 68, 145, 67, 108, 181, 169, 112, 67, 234, 39, 145, 67, 108, 16, 160, 113, 67, 41, 5, 145, 67, 108, 162, 146, 114, 67, 75, 220, 144, 67, 108, 210, 128, 115, 67, 107, 173, 144, 67, 108, 5, 106, 116, 67, 165, 120, 144, 67, 108, 168, 77, 117, 67, 29,
			62, 144, 67, 108, 40, 43, 118, 67, 247, 253, 143, 67, 108, 248, 1, 119, 67, 92, 184, 143, 67, 108, 141, 209, 119, 67, 122, 109, 143, 67, 108, 100, 153, 120, 67, 127, 29, 143, 67, 108, 253, 88, 121, 67, 160, 200, 142, 67, 108, 220, 15, 122, 67, 18, 111, 142, 67, 108, 142, 189, 122, 67, 15, 17, 142, 67, 108, 161, 97, 123,
			67, 211, 174, 141, 67, 108, 175, 251, 123, 67, 156, 72, 141, 67, 108, 84, 139, 124, 67, 173, 222, 140, 67, 108, 52, 16, 125, 67, 73, 113, 140, 67, 108, 251, 137, 125, 67, 181, 0, 140, 67, 108, 89, 248, 125, 67, 59, 141, 139, 67, 108, 10, 91, 126, 67, 36, 23, 139, 67, 108, 205, 177, 126, 67, 186, 158, 138, 67, 108, 107,
			252, 126, 67, 77, 36, 138, 67, 108, 180, 58, 127, 67, 41, 168, 137, 67, 108, 128, 108, 127, 67, 159, 42, 137, 67, 108, 177, 145, 127, 67, 254, 171, 136, 67, 108, 44, 170, 127, 67, 151, 44, 136, 67, 108, 229, 181, 127, 67, 189, 172, 135, 67, 108, 0, 183, 127, 67, 128, 119, 135, 67, 108, 0, 183, 127, 67, 128, 119, 135,
			67, 108, 0, 183, 127, 67, 128, 119, 135, 67, 108, 154, 176, 127, 67, 142, 247, 134, 67, 108, 108, 157, 127, 67, 237, 119, 134, 67, 108, 130, 125, 127, 67, 240, 248, 133, 67, 108, 241, 80, 127, 67, 232, 122, 133, 67, 108, 213, 23, 127, 67, 38, 254, 132, 67, 108, 83, 210, 126, 67, 248, 130, 132, 67, 108, 151, 128, 126,
			67, 175, 9, 132, 67, 108, 213, 34, 126, 67, 151, 146, 131, 67, 108, 74, 185, 125, 67, 253, 29, 131, 67, 108, 57, 68, 125, 67, 44, 172, 130, 67, 108, 237, 195, 124, 67, 108, 61, 130, 67, 108, 184, 56, 124, 67, 4, 210, 129, 67, 108, 243, 162, 123, 67, 57, 106, 129, 67, 108, 254, 2, 123, 67, 78, 6, 129, 67, 108, 64, 89, 122,
			67, 130, 166, 128, 67, 108, 36, 166, 121, 67, 18, 75, 128, 67, 108, 30, 234, 120, 67, 114, 232, 127, 67, 108, 165, 37, 120, 67, 94, 68, 127, 67, 108, 56, 89, 119, 67, 80, 170, 126, 67, 108, 90, 133, 118, 67, 172, 26, 126, 67, 108, 145, 170, 117, 67, 203, 149, 125, 67, 108, 106, 201, 116, 67, 5, 28, 125, 67, 108, 117, 226,
			115, 67, 166, 173, 124, 67, 108, 70, 246, 114, 67, 246, 74, 124, 67, 108, 116, 5, 114, 67, 51, 244, 123, 67, 108, 153, 16, 113, 67, 149, 169, 123, 67, 108, 81, 24, 112, 67, 76, 107, 123, 67, 108, 60, 29, 111, 67, 127, 57, 123, 67, 108, 250, 31, 110, 67, 79, 20, 123, 67, 108, 45, 33, 109, 67, 212, 251, 122, 67, 108, 121,
			33, 108, 67, 28, 240, 122, 67, 108, 0, 183, 107, 67, 0, 239, 122, 67, 108, 0, 183, 107, 67, 0, 239, 122, 67, 99, 109, 0, 128, 99, 67, 0, 203, 128, 67, 108, 0, 0, 122, 67, 192, 73, 135, 67, 108, 0, 128, 99, 67, 128, 200, 141, 67, 108, 0, 128, 99, 67, 0, 203, 128, 67, 99, 101, 0, 0 };
		path.loadPathFromData(pathData, sizeof(pathData));
		break;
	}
	default: jassertfalse;
	}

	DrawablePath *p = new DrawablePath();

	p->setFill(FillType(!isOn ? Colours::white.withAlpha(0.6f) : Colours::white.withAlpha(0.8f)));
	p->setPath(path);

	return p;
}


FileBrowser::FileBrowser(BackendRootWindow* rootWindow_) :
	rootWindow(rootWindow_),
	directorySearcher("Directory Scanner")
{
    loadFavoriteFile();
    
	GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).addListener(this);

    if(!rootWindow_->getBackendProcessor()->isFlakyThreadingAllowed())
       directorySearcher.startThread(3);

	fileFilter = new HiseFileBrowserFilter();

	browserCommandManager = new ApplicationCommandManager(); // dynamic_cast<MainController*>(editor->getAudioProcessor())->getCommandManager();

	rootWindow->getMainController()->getExpansionHandler().addListener(this);

	browserToolbarFactory = new FileBrowserToolbarFactory(this);

	addAndMakeVisible(browserToolbar = new Toolbar());

	browserToolbar->addDefaultItems(*browserToolbarFactory);

	browserToolbar->setColour(Toolbar::ColourIds::backgroundColourId, Colours::transparentBlack);
	browserToolbar->setColour(Toolbar::ColourIds::buttonMouseOverBackgroundColourId, Colours::white.withAlpha(0.5f));
	browserToolbar->setColour(Toolbar::ColourIds::buttonMouseDownBackgroundColourId, Colours::white.withAlpha(0.7f));

	addKeyListener(browserCommandManager->getKeyMappings());

	setWantsKeyboardFocus(true);

	browseUndoManager = new UndoManager();

	browserCommandManager->registerAllCommandsForTarget(this);
	browserCommandManager->setFirstCommandTarget(this);
	
	addAndMakeVisible(textEditor = new TextEditor());
	textEditor->setFont(GLOBAL_BOLD_FONT());
	textEditor->addListener(this);
    textEditor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
	textEditor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	textEditor->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.7f));

	directoryList = new DirectoryContentsList(fileFilter, directorySearcher);

	addAndMakeVisible(fileTreeComponent = new FileTreeComponent(*directoryList));

	setName("File Browser");
	fileTreeComponent->setDragAndDropDescription("Internal File Browser");

	fileTreeComponent->setMultiSelectEnabled(true);

	fileTreeComponent->setLookAndFeel(&laf);

	fileTreeComponent->setColour(FileTreeComponent::backgroundColourId, Colours::transparentBlack);
	fileTreeComponent->setColour(FileTreeComponent::linesColourId, Colours::white);
	fileTreeComponent->setColour(FileTreeComponent::selectedItemBackgroundColourId, Colours::black.withAlpha(0.4f));
    fileTreeComponent->setColour(FileTreeComponent::textColourId, Colours::white);

	fileTreeComponent->addMouseListener(this, true);

#if HISE_IOS
#else
    goToDirectory(GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).getWorkDirectory());
#endif

	browseUndoManager->clearUndoHistory();
}

void FileBrowser::projectChanged(const File& newRootDirectory)
{
	goToDirectory(newRootDirectory, false);

	browseUndoManager->clearUndoHistory();
}

void FileBrowser::expansionPackLoaded(Expansion* currentExpansion)
{
	if (currentExpansion == nullptr)
	{
		goToDirectory(GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).getWorkDirectory());
	}
	else
	{
		goToDirectory(currentExpansion->getRootFolder(), false);
	}

	browseUndoManager->clearUndoHistory();
}

void FileBrowser::goToDirectory(const File &newRoot, bool useUndoManager)
{
    textEditor->setText(newRoot.getFullPathName(), dontSendNotification);
    
    if(useUndoManager)
    {
        browseUndoManager->beginNewTransaction();
        browseUndoManager->perform(new UndoableBrowseAction(this, newRoot));
        
    }
    else
    {
        directoryList->setDirectory(newRoot, true, true);
    }
	
	browserCommandManager->commandStatusChanged();
}

void FileBrowser::getAllCommands(Array<CommandID>& commands)
{
	const CommandID id[] = { ShowFavoritePopup,
		AddFavorite,
		RemoveFavorite,
		HardDisks,
		Back,
		Forward
	};

	commands.addArray(id, numElementsInArray(id));
}

void FileBrowser::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	switch (commandID)
	{
	case HardDisks:
		result.setInfo("Show Harddisks", "Show Harddisks", "View", 0);

		
		break;
	case ShowFavoritePopup:
		result.setInfo("Go to Project Root folder", "Go to project root folder", "View", 0);
		
		result.addDefaultKeypress(KeyPress::escapeKey, ModifierKeys::noModifiers);
		break;
	case AddFavorite:
		result.setInfo("Add to Favorites", "And current root directory to Favorites", "View", 0);
		result.setTicked(false);
		result.setActive(true);
		break;
	case RemoveFavorite:
		result.setInfo("Remove from Favorites", "Remove Favorite Directories", "View", 0);
		
		break;
	case Back:
		result.setInfo("Browse Back", "See the last directory", "View", 0);
		result.setActive(browseUndoManager->canUndo());
		result.addDefaultKeypress(KeyPress::backspaceKey, ModifierKeys::noModifiers);
		break;
	case Forward:
		result.setInfo("Browse Forward", "Browse forward", "View", 0);
		result.setActive(browseUndoManager->canRedo());
		break;
	default:
		jassertfalse;
		break;
	}
}



bool FileBrowser::perform(const InvocationInfo &info)
{
	switch (info.commandID)
	{
	case ShowFavoritePopup:
	{
		goToDirectory(GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).getWorkDirectory());

		return true;
	}
	case HardDisks:
	{
		PopupLookAndFeel mlaf;
		PopupMenu m;
		m.setLookAndFeel(&mlaf);

		return true;
	}
	case AddFavorite:
            favorites.add(new Favorite(PresetHandler::getCustomName("Favorite"), directoryList->getDirectory()));
		return true;
	case RemoveFavorite:
            
		return true;
	case Back:
		browseUndoManager->undo();
		return true;
	case Forward:
		browseUndoManager->redo();
		return true;
	}

	return false;
}

void FileBrowser::paint(Graphics &g)
{
	if (getHeight() <= 0)
		return;

	g.setColour(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

	g.fillRect(0, 24, getWidth(), getHeight() - 24);

	g.setColour(Colour(DEBUG_AREA_BACKGROUND_COLOUR_DARK));

	g.fillRect(0.0f, 0.0f, (float)getWidth(), 24.0f);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f), 0.0f, 24.0f,
		Colours::transparentBlack, 0.0f, 30.0f, false));

	g.fillRect(0.0f, 24.0f, (float)getWidth(), 25.0f);

	if (directoryList->getNumFiles() == 0)
	{
		g.setColour(Colours::white.withAlpha(0.5f));
		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText("This directory is empty", getLocalBounds(), Justification::centred);
	}
}

//class FilePreviewComponent

void FileBrowser::previewFile(const File& f)
{
	if (currentlyPreviewFile == f)
	{
		rootWindow->getRootFloatingTile()->showComponentInRootPopup(nullptr, nullptr, juce::Point<int>());
		currentlyPreviewFile = File();
		return;
	}

	currentlyPreviewFile = f;

	auto ff = dynamic_cast<HiseFileBrowserFilter*>(fileFilter.get());

	Component* content = nullptr;

	if (ff->isImageFile(f))
	{
		auto ipc = new ImagePreviewComponent();
		
		ipc->setSize(500, 500);

		ipc->selectedFileChanged(f);

		content = ipc;
	}
	else if (ff->isScriptFile(f))
	{
		auto c = new JSONEditor(f);

		c->setSize(600, 500);

		content = c;
	}
	else
	{
		return;
	}

	auto s = fileTreeComponent->getSelectedItem(0);

	Rectangle<int> bounds = Rectangle<int>();

	if (s != nullptr)
	{
		bounds = s->getItemPosition(true);
	}

	rootWindow->getRootFloatingTile()->showComponentInRootPopup(content, fileTreeComponent, bounds.getCentre());
}

FileBrowser::~FileBrowser()
{
	//GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).removeListener(this);

	rootWindow->getMainController()->getExpansionHandler().removeListener(this);

    saveFavoriteFile();
    
    fileTreeComponent = nullptr;
    fileFilter = nullptr;
    
   	directoryList = nullptr;

	
}

void FileBrowser::resized()
{
	browserToolbar->setBounds(2, 2, getWidth()-4, 20);
	textEditor->setBounds(3 * 20 + 4, 0, getWidth() - 3 * 20 - 4, 24);
	fileTreeComponent->setBounds(0, 24, getWidth(), getHeight() - 24);
}



void FileBrowser::mouseDown(const MouseEvent& e)
{
	RETURN_WHEN_X_BUTTON();

	if (e.mods.isLeftButtonDown())
	{
		StringArray sa;

		for (int i = 0; i < fileTreeComponent->getNumSelectedFiles(); i++)
		{
			sa.add(fileTreeComponent->getSelectedFile(i).getFullPathName());
		}

		fileTreeComponent->setDragAndDropDescription(sa.joinIntoString(";"));
	}
    else
    {
        PopupMenu m;
        
        m.setLookAndFeel(&plaf);
        
#if JUCE_WINDOWS
		m.addItem(1, "Show in Explorer"); // Wow, cross platform overload...
#else
        m.addItem(1, "Show in Finder");
#endif
        m.addItem(2, "Copy as reference");
        
        const int result = m.show();
        
        if(result == 1)
        {
            if(fileTreeComponent->getNumSelectedFiles() > 0)
            {
                fileTreeComponent->getSelectedFile(0).revealToUser();
            }
        }
        else if (result == 2)
        {
            
        }
        
    }
	
}

void FileBrowser::mouseDoubleClick(const MouseEvent& )
{
	File newRoot = fileTreeComponent->getSelectedFile();

    auto *rw = GET_BACKEND_ROOT_WINDOW(this);
    
	if (newRoot.isDirectory())
	{
		goToDirectory(newRoot);
	}
	else if (newRoot.getFileName() == "LinkWindows" || newRoot.getFileName() == "LinkOSX")
	{
		auto target = newRoot.loadFileAsString();

		if (target.contains("{GLOBAL_SAMPLE_FOLDER}"))
		{
			auto path = dynamic_cast<GlobalSettingManager*>(rw->getMainController())->getSettingsObject().getSetting(HiseSettings::Other::GlobalSamplePath).toString();

			File gsd(path);

			target = gsd.getChildFile(target.fromFirstOccurrenceOf("{GLOBAL_SAMPLE_FOLDER}", false, false)).getFullPathName();
		}

		goToDirectory(File(target));
	}
	else if (newRoot.getFileExtension() == ".hip")
	{
		rw->getMainPanel()->loadNewContainer(newRoot);
	}
    else if (newRoot.getFileExtension() == ".js")
    {
        // First look if the script is already used
        
        Processor::Iterator<JavascriptProcessor> iter(rw->getMainSynthChain());
        
        while (JavascriptProcessor *sp = iter.getNextProcessor())
        {
            for (int i = 0; i < sp->getNumWatchedFiles(); i++)
            {
                if (sp->getWatchedFile(i) == newRoot)
                {
                    sp->showPopupForFile(i);
                    return;
                }
            }
        }
    }
    else if ((ImageFileFormat::findImageFormatForFileExtension(newRoot) != nullptr) || (newRoot.getFileExtension() == ".ttf"))
    {
        const String reference = GET_PROJECT_HANDLER(rootWindow->getMainSynthChain()).getFileReference(newRoot.getFullPathName(), ProjectHandler::SubDirectories::Images);
        
        rootWindow->getMainSynthChain()->getMainController()->insertStringAtLastActiveEditor("\"" + reference + "\"", false);
    }
}

void FileBrowser::textEditorReturnKeyPressed(TextEditor& editor)
{
	if (&editor == textEditor)
	{
		File newRoot = File(editor.getText());

		if (newRoot.isDirectory())
		{
			goToDirectory(newRoot, true);
		}
	}
}

bool FileBrowser::keyPressed(const KeyPress& key)
{
	if (key.getKeyCode() == KeyPress::spaceKey)
	{
		previewFile(fileTreeComponent->getSelectedFile(0));

		return fileTreeComponent->getNumSelectedFiles() > 0;
	}
	if (key.isKeyCode(KeyPress::upKey) || key.isKeyCode(KeyPress::downKey))
	{
		if (fileTreeComponent->getNumSelectedFiles() == 1)
		{
			fileTreeComponent->moveSelectedRow(key.isKeyCode(KeyPress::downKey) ? 1 : -1);
		}
		return true;
	}
	else if (key.isKeyCode(KeyPress::rightKey) && fileTreeComponent->getSelectedFile(0).isDirectory())
	{
		if (fileTreeComponent->getNumSelectedItems() == 1)
		{
			fileTreeComponent->getSelectedItem(0)->setOpen(true);
		}
		return true;
	}
	else if (key.isKeyCode(KeyPress::returnKey) && fileTreeComponent->getSelectedFile(0).isDirectory())
	{
		if (fileTreeComponent->getNumSelectedItems() == 1)
		{
			goToDirectory(fileTreeComponent->getSelectedFile(), true);
		}
		return true;
	}
	else if (key.isKeyCode(65) && key.getModifiers().isCommandDown())
	{
		
		fileTreeComponent->deselectAllFiles();
		for (int i = 0; i < fileTreeComponent->getNumRowsInTree(); i++)
		{
            if(fileTreeComponent->getItemOnRow(i)->mightContainSubItems()) continue;
            
			fileTreeComponent->getItemOnRow(i)->setSelected(true, false, sendNotificationAsync);
		}

		return true;
	}


	return false;
}

} // namespace hise
