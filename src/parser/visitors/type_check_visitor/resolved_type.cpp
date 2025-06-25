#include "resolved_type.h"
#include <sstream>

namespace visitors {

// Static factory methods for creating different types
ResolvedType ResolvedType::Void() {
  ResolvedType type(TypeKind::Void);
  return type;
}

ResolvedType ResolvedType::Int() {
  ResolvedType type(TypeKind::Int);
  return type;
}

ResolvedType ResolvedType::Float() {
  ResolvedType type(TypeKind::Float);
  return type;
}

ResolvedType ResolvedType::Bool() {
  ResolvedType type(TypeKind::Bool);
  return type;
}

ResolvedType ResolvedType::String() {
  ResolvedType type(TypeKind::String);
  return type;
}

ResolvedType ResolvedType::Named(const std::string &name) {
  ResolvedType type(TypeKind::Named);
  type.name_ = name;
  return type;
}

ResolvedType ResolvedType::Array(std::shared_ptr<ResolvedType> elementType) {
  ResolvedType type(TypeKind::Array);
  type.elementType_ = std::move(elementType);
  return type;
}

ResolvedType ResolvedType::Pointer(std::shared_ptr<ResolvedType> pointeeType,
                                   bool isUnsafe) {
  ResolvedType type(TypeKind::Pointer);
  type.pointeeType_ = std::move(pointeeType);
  type.isUnsafe_ = isUnsafe;
  return type;
}

ResolvedType ResolvedType::Reference(std::shared_ptr<ResolvedType> refType) {
  ResolvedType type(TypeKind::Reference);
  type.pointeeType_ = std::move(refType);
  return type;
}

ResolvedType
ResolvedType::Function(std::shared_ptr<ResolvedType> returnType,
                       std::vector<std::shared_ptr<ResolvedType>> paramTypes) {
  ResolvedType type(TypeKind::Function);
  type.returnType_ = std::move(returnType);
  type.paramTypes_ = std::move(paramTypes);
  return type;
}

ResolvedType ResolvedType::Smart(std::shared_ptr<ResolvedType> pointeeType,
                                 SmartKind kind) {
  ResolvedType type(TypeKind::Smart);
  type.pointeeType_ = std::move(pointeeType);
  type.smartKind_ = kind;
  return type;
}

ResolvedType ResolvedType::Union(std::shared_ptr<ResolvedType> left,
                                 std::shared_ptr<ResolvedType> right) {
  ResolvedType type(TypeKind::Union);
  type.leftType_ = std::move(left);
  type.rightType_ = std::move(right);
  return type;
}

ResolvedType
ResolvedType::Template(const std::string &name,
                       std::vector<std::shared_ptr<ResolvedType>> args) {
  ResolvedType type(TypeKind::Template);
  type.name_ = name;
  type.templateArgs_ = std::move(args);
  return type;
}

ResolvedType ResolvedType::Error() { return ResolvedType(TypeKind::Error); }

bool ResolvedType::isAssignableTo(const ResolvedType &other) const {
  // If types are identical, they're assignable
  if (equals(other)) {
    return true;
  }

  // If target is Error or source is Error, allow assignment to avoid cascading
  // errors
  if (kind_ == TypeKind::Error || other.kind_ == TypeKind::Error) {
    return true;
  }

  // If target is a union type, check if source is assignable to either part
  if (other.kind_ == TypeKind::Union) {
    return isAssignableTo(*other.leftType_) ||
           isAssignableTo(*other.rightType_);
  }

  // Check for numeric type conversions with potential loss
  if (kind_ == TypeKind::Int && other.kind_ == TypeKind::Float) {
    return true; // Implicit int to float is safe
  }

  // Smart pointer assignability
  if (kind_ == TypeKind::Smart && other.kind_ == TypeKind::Smart) {
    // Check smart pointer compatibility rules
    if (smartKind_ == other.smartKind_) {
      return pointeeType_->isAssignableTo(*other.pointeeType_);
    }

    // Allow shared to weak conversion
    if (smartKind_ == SmartKind::Shared &&
        other.smartKind_ == SmartKind::Weak) {
      return pointeeType_->isAssignableTo(*other.pointeeType_);
    }
  }

  // Arrays can be assigned if their element types are compatible
  if (kind_ == TypeKind::Array && other.kind_ == TypeKind::Array) {
    return elementType_->isAssignableTo(*other.elementType_);
  }

  // Function types can be assigned if they're compatible (contravariant params,
  // covariant return)
  if (kind_ == TypeKind::Function && other.kind_ == TypeKind::Function) {
    // Check return type compatibility (covariant)
    if (!returnType_->isAssignableTo(*other.returnType_)) {
      return false;
    }

    // Check parameter compatibility (contravariant)
    if (paramTypes_.size() != other.paramTypes_.size()) {
      return false;
    }

    for (size_t i = 0; i < paramTypes_.size(); i++) {
      // For parameters, the target type must be assignable to the source type
      // (contravariance)
      if (!other.paramTypes_[i]->isAssignableTo(*paramTypes_[i])) {
        return false;
      }
    }

    return true;
  }

  // By default, types are not assignable
  return false;
}

bool ResolvedType::isImplicitlyConvertibleTo(const ResolvedType &other) const {
  // Direct assignability implies implicit convertibility
  if (isAssignableTo(other)) {
    return true;
  }

  // Specific implicit conversion rules
  if (kind_ == TypeKind::Int && other.kind_ == TypeKind::Float) {
    return true; // int can be implicitly converted to float
  }

  if (kind_ == TypeKind::Int && other.kind_ == TypeKind::Bool) {
    return true; // int can be implicitly converted to bool (0 = false, non-0 =
                 // true)
  }

  if (kind_ == TypeKind::Float && other.kind_ == TypeKind::Bool) {
    return true; // float can be implicitly converted to bool (0.0 = false,
                 // non-0.0 = true)
  }

  // Pointer to bool conversion
  if (kind_ == TypeKind::Pointer && other.kind_ == TypeKind::Bool) {
    return true; // pointer can be implicitly converted to bool (null = false,
                 // non-null = true)
  }

  // Smart pointer to bool conversion
  if (kind_ == TypeKind::Smart && other.kind_ == TypeKind::Bool) {
    return true; // smart pointer can be implicitly converted to bool
  }

  return false;
}

bool ResolvedType::isExplicitlyConvertibleTo(const ResolvedType &other) const {
  // If implicitly convertible, then explicitly convertible too
  if (isImplicitlyConvertibleTo(other)) {
    return true;
  }

  // Numeric conversions
  if ((kind_ == TypeKind::Float && other.kind_ == TypeKind::Int) ||
      (kind_ == TypeKind::Int && other.kind_ == TypeKind::Float)) {
    return true;
  }

  // Basic type to string conversion
  if (other.kind_ == TypeKind::String &&
      (kind_ == TypeKind::Int || kind_ == TypeKind::Float ||
       kind_ == TypeKind::Bool)) {
    return true;
  }

  // Allow explicit conversion between any pointer types
  if (kind_ == TypeKind::Pointer && other.kind_ == TypeKind::Pointer) {
    return true;
  }

  // Allow explicit conversion from any pointer to integer
  if (kind_ == TypeKind::Pointer && other.kind_ == TypeKind::Int) {
    return true;
  }

  // Allow explicit conversion from integer to any pointer
  if (kind_ == TypeKind::Int && other.kind_ == TypeKind::Pointer) {
    return true;
  }

  // Allow explicit conversions between smart pointer types
  if (kind_ == TypeKind::Smart && other.kind_ == TypeKind::Smart) {
    return true;
  }

  // Allow explicit conversion between smart pointer and raw pointer
  if ((kind_ == TypeKind::Smart && other.kind_ == TypeKind::Pointer) ||
      (kind_ == TypeKind::Pointer && other.kind_ == TypeKind::Smart)) {
    return true;
  }

  // Allow explicit conversion between union type and its components
  if (other.kind_ == TypeKind::Union) {
    return isExplicitlyConvertibleTo(*other.leftType_) ||
           isExplicitlyConvertibleTo(*other.rightType_);
  }

  if (kind_ == TypeKind::Union) {
    return leftType_->isExplicitlyConvertibleTo(other) ||
           rightType_->isExplicitlyConvertibleTo(other);
  }

  return false;
}

bool ResolvedType::equals(const ResolvedType &other) const {
  // If the kinds are different, the types are not equal
  if (kind_ != other.kind_) {
    return false;
  }

  // Check equality based on type kind
  switch (kind_) {
  case TypeKind::Void:
  case TypeKind::Int:
  case TypeKind::Float:
  case TypeKind::Bool:
  case TypeKind::String:
  case TypeKind::Error:
    return true; // Basic types of same kind are equal

  case TypeKind::Named:
    return name_ == other.name_;

  case TypeKind::Array:
    return elementType_->equals(*other.elementType_);

  case TypeKind::Pointer:
    return pointeeType_->equals(*other.pointeeType_) &&
           isUnsafe_ == other.isUnsafe_;

  case TypeKind::Reference:
    return pointeeType_->equals(*other.pointeeType_);

  case TypeKind::Function:
    // Check return type
    if (!returnType_->equals(*other.returnType_)) {
      return false;
    }

    // Check parameter types
    if (paramTypes_.size() != other.paramTypes_.size()) {
      return false;
    }

    for (size_t i = 0; i < paramTypes_.size(); i++) {
      if (!paramTypes_[i]->equals(*other.paramTypes_[i])) {
        return false;
      }
    }
    return true;

  case TypeKind::Smart:
    return pointeeType_->equals(*other.pointeeType_) &&
           smartKind_ == other.smartKind_;

  case TypeKind::Union:
    // For unions, order doesn't matter: A|B == B|A
    return (leftType_->equals(*other.leftType_) &&
            rightType_->equals(*other.rightType_)) ||
           (leftType_->equals(*other.rightType_) &&
            rightType_->equals(*other.leftType_));

  case TypeKind::Template:
    if (name_ != other.name_ ||
        templateArgs_.size() != other.templateArgs_.size()) {
      return false;
    }

    for (size_t i = 0; i < templateArgs_.size(); i++) {
      if (!templateArgs_[i]->equals(*other.templateArgs_[i])) {
        return false;
      }
    }
    return true;

  default:
    return false;
  }
}

std::string ResolvedType::toString() const {
  std::ostringstream oss;

  switch (kind_) {
  case TypeKind::Void:
    return "void";
  case TypeKind::Int:
    return "int";
  case TypeKind::Float:
    return "float";
  case TypeKind::Bool:
    return "bool";
  case TypeKind::String:
    return "string";
  case TypeKind::Named:
    return name_;
  case TypeKind::Array:
    return elementType_->toString() + "[]";
  case TypeKind::Pointer:
    if (isUnsafe_) {
      return pointeeType_->toString() + "@unsafe";
    } else {
      return pointeeType_->toString() + "@";
    }
  case TypeKind::Reference:
    return pointeeType_->toString() + "&";
  case TypeKind::Function:
    oss << "function(";
    for (size_t i = 0; i < paramTypes_.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << paramTypes_[i]->toString();
    }
    oss << "): " << returnType_->toString();
    return oss.str();
  case TypeKind::Smart:
    switch (smartKind_) {
    case SmartKind::Shared:
      return "#shared<" + pointeeType_->toString() + ">";
    case SmartKind::Unique:
      return "#unique<" + pointeeType_->toString() + ">";
    case SmartKind::Weak:
      return "#weak<" + pointeeType_->toString() + ">";
    default:
      return "invalid_smart_pointer";
    }
  case TypeKind::Union:
    return leftType_->toString() + " | " + rightType_->toString();
  case TypeKind::Template:
    oss << name_ << "<";
    for (size_t i = 0; i < templateArgs_.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << templateArgs_[i]->toString();
    }
    oss << ">";
    return oss.str();
  case TypeKind::Error:
    return "error_type";
  default:
    return "unknown_type";
  }
}

} // namespace visitors