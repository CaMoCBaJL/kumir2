void main()
{
	bool а;
	а = true;
	
	bool б;
	б = !а;	
	
        bool б1;
        б1 := !(! а);
        
        bool в;
        в := а && б;
        bool в00;
        в00 := ! а && б;
        bool в01; 
        в01 := ! а && ! б;
        bool в11 = (! (а && б));
        bool в12 = (! (! а && б));
        bool в13 = (! (! а && ! б));
        bool в21 = (! (! (а && б)));
        
        bool г;
        г := а || б;
        bool г00 = (! а || б);
        bool г01 = (! а || ! б);
        bool г11 = (! (а || б));
        bool г12 = (! (! а || б));
        bool г13 = (! (! а || ! б));
        bool г21 = (! (! (а || б)));
        return;
}
