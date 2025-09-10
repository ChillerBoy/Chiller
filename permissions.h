
#pragma once
// permissions.h â€” role-based access helpers
enum class Role { VIEWER=0, OPERATOR=1, ADMIN=2, BAS=3 };

inline bool can_view(Role r){ return true; }
inline bool can_write_basic(Role r){ return r==Role::OPERATOR || r==Role::ADMIN || r==Role::BAS; }
inline bool can_override(Role r){ return r==Role::ADMIN || r==Role::BAS; }
inline bool can_test(Role r, bool any_alarm){ 
  if (r==Role::ADMIN || r==Role::BAS) return true;
  if (r==Role::OPERATOR && !any_alarm) return true;
  return false;
}
