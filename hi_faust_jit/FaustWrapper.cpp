

namespace scriptnode {
namespace faust {

bool faust_jit_helpers::isValidClassId(String cid)
{
	if (cid.length() <= 0)
		return false;

	if (!isalpha(cid[0]) && cid[0] != '_')
		return false;

	for (auto c : cid) {
		if (!isalnum(c) && c != '_')
			return false;
	}

	return true;
}

} // namespace faust
} // namespace scriptnode


