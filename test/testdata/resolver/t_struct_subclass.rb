# typed: true

class DIS < T::DeprecatedInheritableStruct; end
class Yep < DIS; end

class S < T::Struct; end
class Nope < S; end # error: Subclassing `S` is not allowed
