# frozen_string_literal: true
# typed: true

class T::DeprecatedInheritableStruct
  include T::Props
  include T::Props::Serializable
  include T::Props::Constructor
end

class T::Struct < T::DeprecatedInheritableStruct
  def self.inherited(subclass)
    super(subclass)
    T::Private::ClassUtils.replace_method(subclass.singleton_class, :inherited) do |s|
      super(s)
      raise "#{self.name} is a subclass of T::Struct and cannot be subclassed"
    end
  end
end
