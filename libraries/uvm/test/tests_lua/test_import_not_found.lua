local M = {}

function M:init()
   pprint('init import not found')
end

function M:start()
   pprint('start import not found')
   local demo2 = import_contract 'not_found'
   if demo2 then
      error("import not found contract should return nil")
   else
      error("successfully import not found contract as nil")
   end
end

return M
