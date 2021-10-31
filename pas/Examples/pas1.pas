program Main;

var y : integer;

procedure Alpha(a : integer; b : integer);
var x : integer;

   procedure Beta(a : integer; b : integer);
   var x : integer;
   begin
      x := a * 10 + b * 2;
      printf(x);
   end;

begin
   x := (a + b ) * 2;
   printf(x,1);

   Beta(5, 10);
end;

begin 

   Alpha(3 + 5, 7);  

   FOR y:=1 TO 2 DO 
   begin
      Alpha(35, y);
   end;

end.  