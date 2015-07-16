
#include "ecosim.h"
//#define VECTORIZE(X,M,Y)   NumericVector (X) = as<NumericVector>((M)[(Y)])
//#define MAKEINT(X,M,Y)     int (X) = as<int>(M[(Y)])

//##############################################################----------
//-----#################################################################----
// [[Rcpp::export]] 
List Adams_test (List params, List instate, List forcing, List fishing, int StartYear, int EndYear){
     
int y, m, dd, sp, i; 

// Parse out List mod
  i=0;
    Rprintf("test %d\n", i++);
  int NUM_LIVING = as<int>(params["NUM_LIVING"]);
    Rprintf("test %d\n", i++);
  int NUM_DEAD   = as<int>(params["NUM_DEAD"]);
    Rprintf("test %d\n", i++);
  int juv_N      = as<int>(params["juv_N"]);
    Rprintf("test %d\n", i++);
  //int init_run   = as<int>(params["init_run"]);
  //  Rprintf("test %d\n", i++);
  int BURN_YEARS = as<int>(params["BURN_YEARS"]);
    Rprintf("test %d\n", i++);
  int CRASH_YEAR = as<int>(params["CRASH_YEAR"]);
    Rprintf("test %d\n", i++);
  int NUM_GROUPS = NUM_LIVING+NUM_DEAD;
  Rprintf("test %d\n", i++);
  NumericVector B_BaseRef        = as<NumericVector>(params["B_BaseRef"]);
  NumericVector NoIntegrate      = as<NumericVector>(params["NoIntegrate"]);
  NumericVector FtimeAdj         = as<NumericVector>(params["FtimeAdj"]);
  NumericVector FtimeQBOpt       = as<NumericVector>(params["FtimeQBOpt"]);
  Rprintf("test %d\n", i++);   
  //NumericMatrix WageS            = as<NumericMatrix>(mod["WageS"]);
  //NumericMatrix NageS            = as<NumericMatrix>(mod["NageS"]);
  //NumericVector stanzaPred       = as<NumericVector>(mod["stanzaPred"]);
  //NumericVector SpawnBio         = as<NumericVector>(mod["SpawnBio"]);
  //NumericVector JuvNum           = as<NumericVector>(mod["JuvNum"]);
  //NumericVector AduNum           = as<NumericVector>(mod["AduNum"]);
  //NumericVector firstMoAdu       = as<NumericVector>(mod["firstMoAdu"]);
  Rprintf("test %d\n", i++);
  //NumericVector FishingLoss      = as<NumericVector>(mod["FishingLoss"]);                      
  NumericMatrix out_BB(EndYear*12+1, NUM_GROUPS+1);           //= as<NumericMatrix>(mod["out_BB"]);
  NumericMatrix out_CC(EndYear*12+1, NUM_GROUPS+1);           //= as<NumericMatrix>(mod["out_CC"]);
  NumericMatrix out_SSB(EndYear*12+1, NUM_GROUPS+1);          //= as<NumericMatrix>(mod["out_SSB"]);
  NumericMatrix out_rec(EndYear*12+1, NUM_GROUPS+1);          //= as<NumericMatrix>(mod["out_rec"]);
  Rprintf("test %d\n", i++); 
   // Update sums of split groups to total biomass for derivative calcs
//NOJUV      SplitSetPred(mod); 
      
    // call derivative twice to set deriv and deriv(t-1)
    // TODO init versus non init_run     if (init_run){
   List state = instate;
   //List dyt0  = deriv_test(params,state,forcing,fishing,0,0,0);
   List dyt   = deriv_test(params,state,forcing,fishing,0,0,0);
  Rprintf("test %d\n", i++);
// MAIN LOOP STARTS HERE
// ASSUMES STEPS_PER_MONTH is 1.0 always, take out divisions     
   for (y = StartYear; y < EndYear; y++){                                  
     Rprintf("Year %d\n",y);
     for (m = 0; m < STEPS_PER_YEAR; m++){     
				 dd = y * STEPS_PER_YEAR + m;  // index for monthly output           
      // save state to output matrix
      // For non-split pools, SSB is output output as the same as B.
			// For adults, it is overwritten with SSB, for juvs 0.     
              //for (sp = 1; sp <= NUM_LIVING + NUM_DEAD; sp++){ 
         NumericVector old_BB    = as<NumericVector>(state["BB"]);
 				 NumericVector old_Ftime = as<NumericVector>(state["Ftime"]);         
 	       NumericVector dydt0     = as<NumericVector>(dyt["DerivT"]);
 	       
 	       dyt   = deriv_test(params,state,forcing,fishing,y,m,0);

         NumericVector dydt1     = as<NumericVector>(dyt["DerivT"]); 
 				 NumericVector FoodGain = as<NumericVector>(dyt["FoodGain"]);					
         NumericVector biomeq   = as<NumericVector>(dyt["biomeq"]);
//NOJUV  NumericVector pd = ifelse(NoIntegrate<0, stanzaPred, old_B);
         NumericVector pd = old_BB;         
         NumericVector new_Ftime =    ifelse( 
                                         (FoodGain>0)&(pd>0),
                                            0.1 + 0.9*old_Ftime * 
                                            ((1.0-FtimeAdj) + FtimeAdj * 
                                            FtimeQBOpt/(FoodGain/pd)),
                                      old_Ftime);
                                                         										                       
         NumericVector new_BB = 
                         ifelse( NoIntegrate==0,
                          (1.0-SORWT)* biomeq + SORWT*old_BB,
                          ifelse(NoIntegrate==sp,
                            old_BB + (DELTA_T/2.0) * (3.0*(dydt1 - dydt0)), 
                          0) 
                         );
                                                               										       
						      // If the new biomass goes to infinity or something, set a
									// flag to exit the loop and return the problem. 
						         if ( any(is_na(new_BB)) | any(is_infinite(new_BB)) ) {
                        CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;
                     }
                  
									// If the run is during the "burn-in" years, and biomass goes
									// into the discard range, set flag to exit the loop.  
//NOJUV                      if (y < BURN_YEARS){
//NOJUV                        if  ( (new_B < B_BaseRef[sp] * LO_DISCARD) ||
//NOJUV                              (new_B > B_BaseRef[sp] * HI_DISCARD) ){
//NOJUV                            CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;                         
//NOJUV                        }                          
//NOJUV                      }
                    
                 // Otherwise if biomass goes very large or very small, set to 
                 // min or max. 
								    //MIN_THRESHOLD(new_B, B_BaseRef[sp] * EPSILON);
                    //MAX_THRESHOLD(new_B, B_BaseRef[sp] * BIGNUM);                    
   state["BB"]    = pmax(pmin(new_BB, B_BaseRef*BIGNUM), B_BaseRef*EPSILON); 
   state["Ftime"] = pmin(new_Ftime, 2.0);                   
 										//state_BB[sp] = new_B;                                
 						        
                 // sum up Catch at every time step (catch is cumulative)
//NOCATCH                     if (fabs(state_BB[sp]/old_B-1.0) > EPSILON){
//NOCATCH 								       out_CC(dd, sp) = ((FishingLoss[sp] * DELTA_T) / old_B) *
//NOCATCH 									                       (state_BB[sp] - old_B) /
//NOCATCH 										 	    							 log(state_BB[sp] / old_B);
//NOCATCH 									  }
//NOCATCH 									  else {out_CC(dd, sp) = (FishingLoss[sp] * DELTA_T);
//NOCATCH 								    }		  
  									         
 					//			 } // End of species loop
		    		       
 					//}  // End of days loop
           
          // Monthly Stanza (split pool) update
//NOJUV 					   update_stanzas(mod, y, m + 1);
//NOJUV             SplitSetPred(mod);
           
				 // As above for non-split groups, check for out-of-bounds, numerical
				 // errors, or burn-in model stoppages for juvs and then adults.   
//NOJUV            for (i = 1; i <= juv_N; i++){
//NOJUV                sp = JuvNum[i];
//NOJUV                new_B = state_BB[sp];
//NOJUV						    if (std::isnan(new_B) || std::isinf(new_B)) {
//NOJUV                   CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;
//NOJUV                }                  
//NOJUV                if (y < BURN_YEARS){
//NOJUV                    if  ( (new_B < B_BaseRef[sp] * LO_DISCARD) ||
//NOJUV                          (new_B > B_BaseRef[sp] * HI_DISCARD) ){
//NOJUV                         CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;                           
//NOJUV                    }                          
//NOJUV                }                
//NOJUV                sp    = AduNum[i];
//NOJUV                new_B = state_BB[sp];
//NOJUV						    if (std::isnan(new_B) || std::isinf(new_B)) {
//NOJUV                    CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;
//NOJUV                }     
//NOJUV                if (y < BURN_YEARS){
//NOJUV                   if  ( (new_B < B_BaseRef[sp] * LO_DISCARD) ||
//NOJUV                         (new_B > B_BaseRef[sp] * HI_DISCARD) ){
//NOJUV                       CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;                           
//NOJUV                   }                          
//NOJUV                }                
//NOJUV            }
         out_BB( dd, _) = old_BB;
         out_SSB(dd, _) = old_BB;
         out_rec(dd, _) = old_BB;                           
//NOJUV               for (i = 1; i <= juv_N; i++){
//NOJUV                   out_SSB(dd, JuvNum[i]) = 0.0;
//NOJUV 									out_SSB(dd, AduNum[i]) = SpawnBio[i];
//NOJUV 									out_rec(dd, AduNum[i]) = NageS(firstMoAdu[i], i) * WageS(firstMoAdu[i], i);
//NOJUV 
 		    }  // End of main months loop
     
   }// End of years loop
       
   // If you get to the end and haven't crashed, save biomass in the
   // final time slot before exiting.
//NOCATCH      if (CRASH_YEAR < 0){
//NOCATCH         dd = EndYear * STEPS_PER_YEAR;
//NOCATCH         for (sp = 1; sp <= NUM_LIVING + NUM_DEAD; sp++){ 
//NOCATCH              out_BB(dd, sp)  = state_BB[sp];
//NOCATCH              out_SSB(dd, sp) = state_BB[sp];
//NOCATCH              out_rec(dd, sp) = state_BB[sp];
//NOCATCH         }
//NOCATCH         for (i = 1; i <= juv_N; i++){
//NOCATCH              out_SSB(dd, JuvNum[i]) = 0.0;
//NOCATCH						  out_SSB(dd, AduNum[i]) = SpawnBio[i];
//NOCATCH						  out_rec(dd, AduNum[i]) = NageS(firstMoAdu[i], i) * WageS(firstMoAdu[i], i);
//NOCATCH         }
//NOCATCH      }  
   List outdat = List::create(
               _["out_BB"]=out_BB);      
return(outdat);
} 


//##############################################################----------
// [[Rcpp::export]] 
List deriv_test(List params, List state, List forcing, List fishing, int y, int m, int d){

int sp, links, prey, pred, gr, dest, i;
  double caught;

  // Parse out List mod
  int NUM_GROUPS                 = as<int>(params["NUM_GROUPS"]);
  int NUM_LIVING                 = as<int>(params["NUM_LIVING"]);
  int NUM_DEAD                   = as<int>(params["NUM_DEAD"]);
  int NumPredPreyLinks           = as<int>(params["NumPredPreyLinks"]);
  int NumFishingLinks            = as<int>(params["NumFishingLinks"]);
  int NumDetLinks                = as<int>(params["NumDetLinks"]);
  int juv_N                      = as<int>(params["juv_N"]);
  int COUPLED                    = as<int>(params["COUPLED"]);
 
  // NUM_GROUPS length input vectors
  NumericVector B_BaseRef        = as<NumericVector>(params["B_BaseRef"]);
  NumericVector MzeroMort        = as<NumericVector>(params["MzeroMort"]);
  NumericVector UnassimRespFrac  = as<NumericVector>(params["UnassimRespFrac"]);
  NumericVector ActiveRespFrac   = as<NumericVector>(params["ActiveRespFrac"]);
  NumericVector HandleSelf       = as<NumericVector>(params["HandleSelf"]);
  NumericVector ScrambleSelf     = as<NumericVector>(params["ScrambleSelf"]);
  NumericVector fish_Effort      = as<NumericVector>(params["fish_Effort"]);

  // NumPredPreyLinks Length vectors
  IntegerVector PreyFrom         = as<IntegerVector>(params["PreyFrom"]);
  IntegerVector PreyTo           = as<IntegerVector>(params["PreyTo"]);
   NumericVector QQ               = as<NumericVector>(params["QQ"]);
   NumericVector DD               = as<NumericVector>(params["DD"]);
   NumericVector VV               = as<NumericVector>(params["VV"]);
   NumericVector HandleSwitch     = as<NumericVector>(params["HandleSwitch"]);
   NumericVector PredPredWeight   = as<NumericVector>(params["PredPredWeight"]);
   NumericVector PreyPreyWeight   = as<NumericVector>(params["PreyPreyWeight"]);

  IntegerVector FishFrom         = as<IntegerVector>(params["FishFrom"]);
  IntegerVector FishThrough      = as<IntegerVector>(params["FishThrough"]);
  IntegerVector FishTo           = as<IntegerVector>(params["FishTo"]);
  NumericVector FishQ            = as<NumericVector>(params["FishQ"]);
  IntegerVector DetFrom          = as<IntegerVector>(params["DetFrom"]);
  IntegerVector DetTo            = as<IntegerVector>(params["DetTo"]);
  NumericVector DetFrac          = as<NumericVector>(params["DetFrac"]);

  NumericVector stanzaPred       = as<NumericVector>(params["stanzaPred"]);
  NumericVector stanzaBasePred   = as<NumericVector>(params["stanzaBasePred"]);
  NumericVector JuvNum           = as<NumericVector>(params["JuvNum"]);
  NumericVector AduNum           = as<NumericVector>(params["AduNum"]);

  NumericVector state_BB         = as<NumericVector>(state["BB"]);
  NumericVector state_Ftime      = as<NumericVector>(state["Ftime"]);

  NumericVector TerminalF        = as<NumericVector>(params["TerminalF"]);
  NumericVector TARGET_BIO       = as<NumericVector>(params["TARGET_BIO"]);
  NumericVector TARGET_F         = as<NumericVector>(params["TARGET_F"]);
  NumericVector ALPHA            = as<NumericVector>(params["ALPHA"]);

  NumericMatrix force_byprey     = as<NumericMatrix>(forcing["byprey"]);
  NumericMatrix force_bymort     = as<NumericMatrix>(forcing["bymort"]);
  NumericMatrix force_bysearch   = as<NumericMatrix>(forcing["bysearch"]);
  
  NumericMatrix FORCED_FRATE     = as<NumericMatrix>(fishing["FRATE"]);
  NumericMatrix FORCED_CATCH     = as<NumericMatrix>(fishing["CATCH"]);

  NumericVector TotGain(NUM_GROUPS+1);//          = as<NumericVector>(par["TotGain"]);
  NumericVector TotLoss(NUM_GROUPS+1);          //= as<NumericVector>(par["TotLoss"]);
  NumericVector LossPropToB(NUM_GROUPS+1);      //= as<NumericVector>(par["LossPropToB"]);
  NumericVector LossPropToQ(NUM_GROUPS+1);      //= as<NumericVector>(par["LossPropToQ"]);
  NumericVector DerivT(NUM_GROUPS+1);           //= as<NumericVector>(par["DerivT"]);
  NumericVector biomeq(NUM_GROUPS+1);           //= as<NumericVector>(par["biomeq"]);  
  NumericVector FoodLoss(NUM_GROUPS+1);//[i]       = 0;
  NumericVector FoodGain(NUM_GROUPS+1);//[i]       = 0;
  NumericVector UnAssimLoss(NUM_GROUPS+1);//[i]    = 0;
  NumericVector ActiveRespLoss(NUM_GROUPS+1);//[i] = 0;   
  NumericVector DetritalGain(NUM_GROUPS+1);//[i]   = 0;
  NumericVector FishingGain(NUM_GROUPS+1);//[i]    = 0;
  NumericVector MzeroLoss(NUM_GROUPS+1);//[i]      = 0;
  NumericVector FishingLoss(NUM_GROUPS+1);//[i]    = 0;
  NumericVector DetritalLoss(NUM_GROUPS+1);//[i]   = 0;
  NumericVector FishingThru(NUM_GROUPS+1);//[i]    = 0;
  NumericVector PredSuite(NUM_GROUPS+1);//[i]      = 0;
  NumericVector HandleSuite(NUM_GROUPS+1);//[i]    = 0;  
    
  NumericVector preyYY(NUM_GROUPS+1);//            = as<NumericVector>(par["preyYY"]);
  NumericVector predYY(NUM_GROUPS+1);//            = as<NumericVector>(par["predYY"]);
   //         Rprintf("0j\n");
  //Rprintf("1\n");
  preyYY = state_Ftime * state_BB / B_BaseRef;
  predYY = state_Ftime * state_BB / B_BaseRef;

 // Set functional response biomass for juvenile and adult groups (including foraging time) 
   for (i=1; i<=juv_N; i++){
     if (stanzaBasePred[JuvNum[i]]>0){
       predYY[JuvNum[i]] = state_Ftime[JuvNum[i]] * 
         stanzaPred[JuvNum[i]]/stanzaBasePred[JuvNum[i]];
       predYY[AduNum[i]] = state_Ftime[AduNum[i]] * 
         stanzaPred[AduNum[i]]/stanzaBasePred[AduNum[i]];
     } 
   } 

// UnRoll Vectors
  NumericVector PYY = preyYY[PreyFrom];
  NumericVector PDY = predYY[PreyTo];

//   // add "mediation by search rate" KYA 7/8/08
//  for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
//    //FORCE     predYY[sp] *= force_bysearch(y * STEPS_PER_YEAR + m, sp);
    predYY = predYY * force_bysearch(y * STEPS_PER_YEAR + m,_);
//   }
  //Rprintf("2\n");  
   // Summed predator and prey suites for joint handling time and/or scramble functional response
   for (links=1; links<=NumPredPreyLinks; links++){
     PredSuite[PreyFrom[links]] += predYY[PreyTo[links]  ] * PredPredWeight[links];
     HandleSuite[PreyTo[links]] += preyYY[PreyFrom[links]] * PreyPreyWeight[links];
   }
   // Unroll the suites into a NumPredPrey length vector
   NumericVector PdSuite = PredSuite[PreyFrom];
   NumericVector PySuite = HandleSuite[PreyTo];
   NumericVector Hself   = HandleSelf[PreyTo]; 
   NumericVector Sself   = ScrambleSelf[PreyTo];
   
//   // Main loop to calculate functional response for each predator/prey link
//     // (3) Additive version: primary used and published in Aydin (2004) 
//     // KYA 3/2/2012 setting "COUPLED" to zero means species are density dependent
//     // (based on their own modul) but don't interact otherwise.  This can magically
//     // create and destroy energy in the system but makes them act like a set
//     // of independent surplus production models for comparison purposes 
//     Q =   QQ[links] * predYY[pred] * pow(preyYY[prey], COUPLED * HandleSwitch[links]) *
//       ( DD[links] / ( DD[links] - 1.0 + 
//       pow(HandleSelf[pred] * preyYY[prey]   + 
//       (1. - HandleSelf[pred]) * HandleSuite[pred],
//                                            COUPLED * HandleSwitch[links])) )*
//                                              ( VV[links] / ( VV[links] - 1.0 + 
//                                              ScrambleSelf[pred] * predYY[pred] + 
//                                              (1. - ScrambleSelf[pred]) * PredSuite[prey]) );
  NumericVector Q1 = 
             QQ * PDY * vpow(PYY, HandleSwitch * COUPLED) *
           ( DD / ( DD-1.0 + vpow(Hself*PYY + (1.-Hself)*PySuite, COUPLED*HandleSwitch)) ) *
           ( VV / ( VV-1.0 +      Sself*PDY + (1.-Sself)*PdSuite) );
  
//     // Include any Forcing by prey   
//     Q *= force_byprey(y * STEPS_PER_YEAR + m, prey); 

//  No vector solution here as we need to sum by both links and species 
 for (links=1; links<=NumPredPreyLinks; links++){
      prey = PreyFrom[links];
      pred = PreyTo[links];
  // If model is uncoupled, food loss doesn't change with prey or predator levels.
     if (COUPLED){  FoodLoss[prey]  += Q1[links]; }
     else{  FoodLoss[prey]  += state_BB[prey] * QQ[links]/B_BaseRef[prey]; }
     FoodGain[pred]         += Q1[links];
 }

// By Species Rates 
     UnAssimLoss    = FoodGain  * UnassimRespFrac; 
     ActiveRespLoss = FoodGain  * ActiveRespFrac;  												 
     MzeroLoss      = MzeroMort * state_BB;
   
//   // MOST OF THE FOLLOWING FISHING SPECIFICATION METHODS ARE NOT SUPPORTED
//   // BY THE R-CODE, only fishing by effort (for gear) or by F-rate (for
//   // species) is supported at the end.
//   //
//   // BY CURRENT R-CODE.  ONLY    
//   // RFISH for (gr=NUM_LIVING+NUM_DEAD+1; gr<=NUM_GROUPS; gr++){
//   // RFISH This sets EFFORT by time series of gear-target combinations
//   // RFISH 		   for (gr=NUM_LIVING+NUM_DEAD+1; gr<=NUM_GROUPS; gr++){
//   // RFISH if -1 is an input value, uses TERMINAL F (last non-negative F) 		
//   //RFISH 		   for (gr=NUM_LIVING+NUM_DEAD+1; gr<=NUM_GROUPS; gr++){
//   //RFISH 		       if (y+m+d == 0){fish_Effort[gr]=1.0;}
//   //RFISH 		       else           {fish_Effort[gr]=1.0;} // NOTE DEFAULT!  THIS CAN BE CHANGED TO 1.0
//   //RFISH        // Added 7/8/08 for forced effort
//   //RFISH            if (FORCED_EFFORT[gr][y] > -0.001) 
//   //RFISH 					    {fish_Effort[gr]=FORCED_EFFORT[gr][y];}
//   //RFISH 
//   //RFISH 			     if ((FORCED_TARGET[gr]>0) && (FORCED_CATCH[gr][y]>-EPSILON)){
//   //RFISH 			        totQ = 0.0;
//   //RFISH 			        sp   = FORCED_TARGET[gr];
//   //RFISH 			        for (links=1; links<=NumFishingLinks; links++){
//   //RFISH     					    if ((FishingThrough[links] == gr) && 
//   //RFISH 		    					    (FishingFrom[links]) == sp){
//   //RFISH 				    					totQ += FishingQ[links];
//   //RFISH 									}
//   //RFISH 						  }
//   //RFISH 						  fish_Effort[gr] = FORCED_CATCH[gr][y]/ 
//   //RFISH 							                  (totQ * state_BB[sp]);
//   //RFISH 							if (FORCED_CATCH[gr][y] >= state_BB[sp])
//   //RFISH 							   {fish_Effort[gr] = (1.0-EPSILON)*(state_BB[sp])/ 
//   //RFISH 				    			                  (totQ * state_BB[sp]);}
//   //RFISH 					 }
//   //RFISH 					 // By putting F after catch, Frates override absolute catch
//   //RFISH 			     if ((FORCED_FTARGET[gr]>0) && (FORCED_FRATE[gr][y]>-EPSILON)){
//   //RFISH 			        totQ = 0.0;
//   //RFISH 			        sp   = FORCED_FTARGET[gr];
//   //RFISH 			        for (links=1; links<=NumFishingLinks; links++){
//   //RFISH     					    if ((FishingThrough[links] == gr) && 
//   //RFISH 		    					    (FishingFrom[links]) == sp){
//   //RFISH 				    					totQ += FishingQ[links];
//   //RFISH 									}
//   //RFISH 						  }
//   //RFISH 						  fish_Effort[gr] = FORCED_FRATE[gr][y]/totQ;
//   //RFISH 							//if (FORCED_CATCH[gr][y] >= state_BB[sp])
//   //RFISH 							//   {fish_Effort[gr] = (1.0-EPSILON)*(state_BB[sp])/ 
//   //RFISH 				    //			                  (totQ * state_BB[sp]);}
//   //RFISH 					 }					 
//   //RFISH 					 
//   //RFISH 				   //if ((y==0) && (m==0) && (d==0)){
//   //RFISH 					 //    cout << path_species[gr] << " " << FORCED_TARGET[gr] << " " << path_species[sp] << " " 
//   //RFISH 					 //	      << state_BB[sp] << " " << FORCED_CATCH[gr][y] << " "
//   //RFISH 					 //		      << fish_Effort[gr] << endl;
//   //RFISH 					 //} 					 
//   //RFISH 			 }
//   
  //double oddball;
    // Apply specified Effort by Gear to catch (using Ecopath-set Q)
    for (links=1; links<=NumFishingLinks; links++){
      prey = FishFrom[links];
      gr   = FishThrough[links];
      dest = FishTo[links];
        caught = FishQ[links] *  fish_Effort[gr] * state_BB[prey]; 
      FishingLoss[prey] += caught;
      FishingThru[gr]   += caught;
      FishingGain[dest] += caught;
    }		
//  Rprintf("4\n"); 
//    
// //   //  Special "CLEAN" fisheries assuming q=1, so specified input is Frate
//    for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
   //   caught = FORCED_CATCH(y, sp) + FORCED_FRATE(y, sp) * state_BB[sp];
      // KYA Aug 2011 removed terminal effort option to allow negative fishing pressure 
//         // if (caught <= -EPSILON) {caught = TerminalF[sp] * state_BB[sp];}
//      if (caught >= state_BB[sp]){caught = (1.0 - EPSILON) * (state_BB[sp]);}
//      FishingLoss[sp] += caught;
//      FishingThru[0]  += caught;
//      FishingGain[0]  += caught;
//      TerminalF[sp] = caught/state_BB[sp];
//    }
//    
    // KINKED CONTROL RULE - NEEDS INPUT of TARGET BIOMASS and TARGET CATCH
    double RefBio, maxcaught;
    for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
      if (TARGET_BIO[sp] > EPSILON){
        RefBio    = state_BB[sp] / TARGET_BIO[sp];
        maxcaught = TARGET_F[sp] * state_BB[sp];         
        if      (RefBio > 1.0)           {caught = maxcaught;}
        else if (RefBio >= ALPHA[sp]) {caught = maxcaught * (RefBio - ALPHA[sp])/(1.0 - ALPHA[sp]);}
        else                             {caught = 0.0;}
        FishingLoss[sp] += caught;
        FishingThru[0]  += caught;
        FishingGain[0]  += caught;
        TerminalF[sp] = caught/state_BB[sp];
      }          
    }
//    
    // DETRITUS  - note: check interdetrital flow carefully, have had some issues
    // (check by ensuring equlibrium run stays in equilibrium)
    int liv, det;
    double flow;
    for (links=1; links<=NumDetLinks; links++){
      liv  = DetFrom[links];
      det  = DetTo[links];
      flow = DetFrac[links] * (MzeroLoss[liv] + UnAssimLoss[liv]);
      DetritalGain[det] += flow;
      if (liv > NUM_LIVING) {DetritalLoss[liv] += flow; }
    }
    for (sp=NUM_LIVING+1; sp<=NUM_LIVING+NUM_DEAD; sp++){
      MzeroLoss[sp] = 0.0;
    }  
//   Rprintf("5\n");   
   // Add mortality forcing
//   for (i=1; i<=NUM_DEAD+NUM_LIVING; i++){
//     FoodLoss[i]  *= force_bymort(y * STEPS_PER_YEAR + m, i);
//     MzeroLoss[i] *= force_bymort(y * STEPS_PER_YEAR + m, i);
//   }
   
   // Sum up derivitive parts        
     TotGain = FoodGain + DetritalGain + FishingGain;      
     LossPropToQ = UnAssimLoss + ActiveRespLoss;
     LossPropToB = FoodLoss    + MzeroLoss + FishingLoss  + DetritalLoss; 
     TotLoss = LossPropToQ + LossPropToB;    
     biomeq  = TotGain/(TotLoss/state_BB);
     DerivT  = TotGain - TotLoss; 
     
//     // Set biomeq for "fast equilibrium" of fast variables
//     if (state_BB[i] > 0) {biomeq[i] = TotGain[i] / (TotLoss[i] / state_BB[i]);}          
  
   List deriv = List::create(
     _["preyYY"]=preyYY,
     _["predYY"]=predYY,
     _["TotGain"]=TotGain,
     _["TotLoss"]=TotLoss,
     _["DerivT"]=DerivT,
     _["biomeq"]=biomeq,
     _["LossPropToB"]=LossPropToB, 
     _["LossPropToQ"]=LossPropToQ,                           
     _["FoodLoss"]=FoodLoss,
     _["FoodGain"]=FoodGain,
     _["UnAssimLoss"]=UnAssimLoss,
     _["ActiveRespLoss"]=ActiveRespLoss,
     _["DetritalGain"]=DetritalGain,
     _["FishingGain"]=FishingGain,
     _["MzeroLoss"]=MzeroLoss,
     _["FishingLoss"]=FishingLoss,
     _["DetritalLoss"]=DetritalLoss,
     _["FishingThru"]=FishingThru,
     _["PredSuite"]=PredSuite,
     _["HandleSuite"]=HandleSuite);

   return(deriv);
}




//-----#################################################################---- 
// Deriv master calculates the biomass dynamics derivative
// [[Rcpp::export]]
int deriv_master(List mod, int y, int m, int d){
  //if (!mod.inherits("Rpath.sim")) stop("Input must be a Rpath model");
  
  // Functional response vars     
  int sp, links, prey, pred, i;
  double caught, Q;
  // Fishing vars
  int gr, dest;
  
  // Parse out List mod
  int NUM_GROUPS                 = as<int>(mod["NUM_GROUPS"]);
  int NUM_LIVING                 = as<int>(mod["NUM_LIVING"]);
  int NUM_DEAD                   = as<int>(mod["NUM_DEAD"]);
  int NumPredPreyLinks           = as<int>(mod["NumPredPreyLinks"]);
  int NumFishingLinks            = as<int>(mod["NumFishingLinks"]);
  int NumDetLinks                = as<int>(mod["NumDetLinks"]);
  int juv_N                      = as<int>(mod["juv_N"]);
  int COUPLED                    = as<int>(mod["COUPLED"]);
  
  NumericVector B_BaseRef        = as<NumericVector>(mod["B_BaseRef"]);
  NumericVector MzeroMort        = as<NumericVector>(mod["MzeroMort"]);
  NumericVector UnassimRespFrac  = as<NumericVector>(mod["UnassimRespFrac"]);
  NumericVector ActiveRespFrac   = as<NumericVector>(mod["ActiveRespFrac"]);
  NumericVector fish_Effort      = as<NumericVector>(mod["fish_Effort"]);
  NumericVector HandleSelf       = as<NumericVector>(mod["HandleSelf"]);
  NumericVector ScrambleSelf     = as<NumericVector>(mod["ScrambleSelf"]);
  NumericVector PreyFrom         = as<NumericVector>(mod["PreyFrom"]);
  NumericVector PreyTo           = as<NumericVector>(mod["PreyTo"]);
  NumericVector QQ               = as<NumericVector>(mod["QQ"]);
  NumericVector DD               = as<NumericVector>(mod["DD"]);
  NumericVector VV               = as<NumericVector>(mod["VV"]);
  NumericVector HandleSwitch     = as<NumericVector>(mod["HandleSwitch"]);
  NumericVector PredPredWeight   = as<NumericVector>(mod["PredPredWeight"]);
  NumericVector PreyPreyWeight   = as<NumericVector>(mod["PreyPreyWeight"]);
  NumericVector FishFrom         = as<NumericVector>(mod["FishFrom"]);
  NumericVector FishThrough      = as<NumericVector>(mod["FishThrough"]);
  NumericVector FishQ            = as<NumericVector>(mod["FishQ"]);
  NumericVector FishTo           = as<NumericVector>(mod["FishTo"]);
  NumericVector DetFrac          = as<NumericVector>(mod["DetFrac"]);
  NumericVector DetFrom          = as<NumericVector>(mod["DetFrom"]);
  NumericVector DetTo            = as<NumericVector>(mod["DetTo"]);
  NumericVector state_BB         = as<NumericVector>(mod["state_BB"]);
  NumericVector state_Ftime      = as<NumericVector>(mod["state_Ftime"]);
  NumericVector stanzaPred       = as<NumericVector>(mod["stanzaPred"]);
  NumericVector stanzaBasePred   = as<NumericVector>(mod["stanzaBasePred"]);
  NumericVector JuvNum           = as<NumericVector>(mod["JuvNum"]);
  NumericVector AduNum           = as<NumericVector>(mod["AduNum"]);
  NumericVector TotGain          = as<NumericVector>(mod["TotGain"]);
  NumericVector TotLoss          = as<NumericVector>(mod["TotLoss"]);
  NumericVector LossPropToB      = as<NumericVector>(mod["LossPropToB"]);
  NumericVector LossPropToQ      = as<NumericVector>(mod["LossPropToQ"]);
  NumericVector DerivT           = as<NumericVector>(mod["DerivT"]);
  NumericVector dyt              = as<NumericVector>(mod["dyt"]);
  NumericVector biomeq           = as<NumericVector>(mod["biomeq"]);
  NumericVector FoodGain         = as<NumericVector>(mod["FoodGain"]);
  NumericVector DetritalGain     = as<NumericVector>(mod["DetritalGain"]);     
  NumericVector FoodLoss         = as<NumericVector>(mod["FoodLoss"]);
  NumericVector UnAssimLoss      = as<NumericVector>(mod["UnAssimLoss"]);
  NumericVector ActiveRespLoss   = as<NumericVector>(mod["ActiveRespLoss"]);   
  NumericVector FishingGain      = as<NumericVector>(mod["FishingGain"]);
  NumericVector MzeroLoss        = as<NumericVector>(mod["MzeroLoss"]);
  NumericVector FishingLoss      = as<NumericVector>(mod["FishingLoss"]);
  NumericVector DetritalLoss     = as<NumericVector>(mod["DetritalLoss"]);
  NumericVector FishingThru      = as<NumericVector>(mod["FishingThru"]);
  NumericVector PredSuite        = as<NumericVector>(mod["PredSuite"]);
  NumericVector HandleSuite      = as<NumericVector>(mod["HandleSuite"]);
  NumericVector preyYY           = as<NumericVector>(mod["preyYY"]);
  NumericVector predYY           = as<NumericVector>(mod["predYY"]);
  NumericVector TerminalF        = as<NumericVector>(mod["TerminalF"]);
  NumericVector TARGET_BIO       = as<NumericVector>(mod["TARGET_BIO"]);
  NumericVector TARGET_F         = as<NumericVector>(mod["TARGET_F"]);
  NumericVector ALPHA            = as<NumericVector>(mod["ALPHA"]);
  
  NumericMatrix force_byprey     = as<NumericMatrix>(mod["force_byprey"]);
  NumericMatrix force_bymort     = as<NumericMatrix>(mod["force_bymort"]);
  NumericMatrix force_bysearch   = as<NumericMatrix>(mod["force_bysearch"]);
  NumericMatrix FORCED_FRATE     = as<NumericMatrix>(mod["FORCED_FRATE"]);
  NumericMatrix FORCED_CATCH     = as<NumericMatrix>(mod["FORCED_CATCH"]);
  
  // Some derivative parts need to be set to zero
  for(i = 0; i < NUM_GROUPS + 1; i++){
    FoodLoss[i]       = 0;
    FoodGain[i]       = 0;
    UnAssimLoss[i]    = 0;
    ActiveRespLoss[i] = 0;   
    DetritalGain[i]   = 0;
    FishingGain[i]    = 0;
    MzeroLoss[i]      = 0;
    FishingLoss[i]    = 0;
    DetritalLoss[i]   = 0;
    FishingThru[i]    = 0;
    PredSuite[i]      = 0;
    HandleSuite[i]    = 0;
  }
			     			  
  //  Set YY = B/B(ref) for functional response; note that foraging time is
	//  used here as a biomass modifier before the main functional response  
  for (sp = 1; sp <= NUM_LIVING + NUM_DEAD; sp++){
    preyYY[sp] = state_Ftime[sp] * state_BB[sp] / B_BaseRef[sp];
 		predYY[sp] = state_Ftime[sp] * state_BB[sp] / B_BaseRef[sp];
     }    
  // The sun always has a biomass of 1 for Primary Production
	preyYY[0] = 1.0;  predYY[0] = 1.0;
 		 		
 		// Set functional response biomass for juvenile and adult groups (including foraging time) 
 		   for (i=1; i<=juv_N; i++){
 		     if (stanzaBasePred[JuvNum[i]]>0){
 		        predYY[JuvNum[i]] = state_Ftime[JuvNum[i]] * 
 				       stanzaPred[JuvNum[i]]/stanzaBasePred[JuvNum[i]];
 		        predYY[AduNum[i]] = state_Ftime[AduNum[i]] * 
 				        stanzaPred[AduNum[i]]/stanzaBasePred[AduNum[i]];
 		     }
 		   }
    // add "mediation by search rate" KYA 7/8/08
        for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
            predYY[sp] *= force_bysearch(y * STEPS_PER_YEAR + m, sp); 
        }
        
 	  // Summed predator and prey for joint handling time and/or scramble functional response
 	     for (links=1; links<=NumPredPreyLinks; links++){
 		          prey = PreyFrom[links];
 		          pred = PreyTo[links];
               PredSuite[prey]   += predYY[pred] * PredPredWeight[links];
               HandleSuite[pred] += preyYY[prey] * PreyPreyWeight[links];
 		  }


	 // Main loop to calculate functional response for each predator/prey link
      for (links=1; links<=NumPredPreyLinks; links++){
 		      prey = PreyFrom[links];
 		      pred = PreyTo[links];
 
      // MAIN FUNCTIONAL RESPONSE SET HERE TO OUTPUT TOTAL CONSUMPTION (Q); use 1 version only
         // (1) This is EwE Classic (c)(r)(tm)(4.0beta)(all rights reserved)
 				 //  Master_Density = predYY[pred];	 
     		 //   Q = QQ[links] * XX[links] * predYY[pred] * preyYY[prey] / 
         //       (Master_Density + ((XX[links] - 1.0) * 
 				 //	                   (1.0 + state_Ftime[prey]) / 2.0) );
  
         // (2) Multiplicative version which is too weird for me.
         // Q =   QQ[links] * 
         //     ( XX[links] * predYY[pred] / ((XX[links] - 1.0) + predYY[pred])     )*
         //     ( HH[links] * preyYY[prey] / ((HH[links] - 1.0) + preyYY[prey])     )*
 				 // 		( DD[links]                 / ((DD[links] - 1.0) + HandleSuite[pred]) )*
 				 // 		( VV[links]                 / ((VV[links] - 1.0) + PredSuite[prey])   );
         
 				 // (3) Additive version: primary used and published in Aydin (2004) 
            // KYA 3/2/2012 setting "COUPLED" to zero means species are density dependent
            // (based on their own modul) but don't interact otherwise.  This can magically
            // create and destroy energy in the system but makes them act like a set
            // of independent surplus production models for comparison purposes 
         Q =   QQ[links] * predYY[pred] * pow(preyYY[prey], COUPLED * HandleSwitch[links]) *
 				      ( DD[links] / ( DD[links] - 1.0 + 
 						                     pow(HandleSelf[pred] * preyYY[prey]   + 
 																 (1. - HandleSelf[pred]) * HandleSuite[pred],
                                                      COUPLED * HandleSwitch[links])) )*
             ( VV[links] / ( VV[links] - 1.0 + 
                                  ScrambleSelf[pred] * predYY[pred] + 
 						                     (1. - ScrambleSelf[pred]) * PredSuite[prey]) );
        
			 // Include any Forcing by prey   
           Q *= force_byprey(y * STEPS_PER_YEAR + m, prey); 

			 // If model is uncoupled, food loss doesn't change with prey or predator levels.
				if (COUPLED){  FoodLoss[prey]  += Q; }
				           else{  FoodLoss[prey]  += state_BB[prey] * QQ[links]/B_BaseRef[prey]; }
        
			 // Energy Accounting
				  FoodGain[pred]           += Q;
          UnAssimLoss[pred]        += Q * UnassimRespFrac[pred]; 
          ActiveRespLoss[pred]     += Q * ActiveRespFrac[pred];  												 
 		  }
 		
     // Mzero Mortality
     for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
         MzeroLoss[sp] = MzeroMort[sp] * state_BB[sp];
     }	   
 		
		 
		// MOST OF THE FOLLOWING FISHING SPECIFICATION METHODS ARE NOT SUPPORTED
		// BY THE R-CODE, only fishing by effort (for gear) or by F-rate (for
		// species) is supported at the end.
		//
		// BY CURRENT R-CODE.  ONLY    
 		// RFISH for (gr=NUM_LIVING+NUM_DEAD+1; gr<=NUM_GROUPS; gr++){
    // RFISH This sets EFFORT by time series of gear-target combinations
 		// RFISH 		   for (gr=NUM_LIVING+NUM_DEAD+1; gr<=NUM_GROUPS; gr++){
    // RFISH if -1 is an input value, uses TERMINAL F (last non-negative F) 		
     //RFISH 		   for (gr=NUM_LIVING+NUM_DEAD+1; gr<=NUM_GROUPS; gr++){
     //RFISH 		       if (y+m+d == 0){fish_Effort[gr]=1.0;}
     //RFISH 		       else           {fish_Effort[gr]=1.0;} // NOTE DEFAULT!  THIS CAN BE CHANGED TO 1.0
     //RFISH        // Added 7/8/08 for forced effort
     //RFISH            if (FORCED_EFFORT[gr][y] > -0.001) 
     //RFISH 					    {fish_Effort[gr]=FORCED_EFFORT[gr][y];}
     //RFISH 
     //RFISH 			     if ((FORCED_TARGET[gr]>0) && (FORCED_CATCH[gr][y]>-EPSILON)){
     //RFISH 			        totQ = 0.0;
     //RFISH 			        sp   = FORCED_TARGET[gr];
     //RFISH 			        for (links=1; links<=NumFishingLinks; links++){
     //RFISH     					    if ((FishingThrough[links] == gr) && 
     //RFISH 		    					    (FishingFrom[links]) == sp){
     //RFISH 				    					totQ += FishingQ[links];
     //RFISH 									}
     //RFISH 						  }
     //RFISH 						  fish_Effort[gr] = FORCED_CATCH[gr][y]/ 
     //RFISH 							                  (totQ * state_BB[sp]);
     //RFISH 							if (FORCED_CATCH[gr][y] >= state_BB[sp])
     //RFISH 							   {fish_Effort[gr] = (1.0-EPSILON)*(state_BB[sp])/ 
     //RFISH 				    			                  (totQ * state_BB[sp]);}
     //RFISH 					 }
     //RFISH 					 // By putting F after catch, Frates override absolute catch
     //RFISH 			     if ((FORCED_FTARGET[gr]>0) && (FORCED_FRATE[gr][y]>-EPSILON)){
     //RFISH 			        totQ = 0.0;
     //RFISH 			        sp   = FORCED_FTARGET[gr];
     //RFISH 			        for (links=1; links<=NumFishingLinks; links++){
     //RFISH     					    if ((FishingThrough[links] == gr) && 
     //RFISH 		    					    (FishingFrom[links]) == sp){
     //RFISH 				    					totQ += FishingQ[links];
     //RFISH 									}
     //RFISH 						  }
     //RFISH 						  fish_Effort[gr] = FORCED_FRATE[gr][y]/totQ;
     //RFISH 							//if (FORCED_CATCH[gr][y] >= state_BB[sp])
     //RFISH 							//   {fish_Effort[gr] = (1.0-EPSILON)*(state_BB[sp])/ 
     //RFISH 				    //			                  (totQ * state_BB[sp]);}
     //RFISH 					 }					 
     //RFISH 					 
     //RFISH 				   //if ((y==0) && (m==0) && (d==0)){
     //RFISH 					 //    cout << path_species[gr] << " " << FORCED_TARGET[gr] << " " << path_species[sp] << " " 
     //RFISH 					 //	      << state_BB[sp] << " " << FORCED_CATCH[gr][y] << " "
     //RFISH 					 //		      << fish_Effort[gr] << endl;
     //RFISH 					 //} 					 
     //RFISH 			 }
 			 					 			 					 
   // Apply specified Effort by Gear to catch (using Ecopath-set Q)
      for (links=1; links<=NumFishingLinks; links++){
 				 prey = FishFrom[links];
 				 gr   = FishThrough[links];
 				 dest = FishTo[links];
 				 caught = FishQ[links] * fish_Effort[gr] * state_BB[prey]; 
          FishingLoss[prey] += caught;
          FishingThru[gr]   += caught;
          FishingGain[dest] += caught;
 		}		
 
    //  Special "CLEAN" fisheries assuming q=1, so specified input is Frate
        for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
             caught = FORCED_CATCH(y, sp) + FORCED_FRATE(y, sp) * state_BB[sp];
             // KYA Aug 2011 removed terminal effort option to allow negative fishing pressure 
                // if (caught <= -EPSILON) {caught = TerminalF[sp] * state_BB[sp];}
             if (caught >= state_BB[sp]){caught = (1.0 - EPSILON) * (state_BB[sp]);}
             FishingLoss[sp] += caught;
             FishingThru[0]  += caught;
             FishingGain[0]  += caught;
             TerminalF[sp] = caught/state_BB[sp];
        }
    
    // KINKED CONTROL RULE - NEEDS INPUT of TARGET BIOMASS and TARGET CATCH
        double RefBio, maxcaught;
        for (sp=1; sp<=NUM_LIVING+NUM_DEAD; sp++){
            if (TARGET_BIO[sp] > EPSILON){
               RefBio    = state_BB[sp] / TARGET_BIO[sp];
               maxcaught = TARGET_F[sp] * state_BB[sp];         
               if      (RefBio > 1.0)           {caught = maxcaught;}
               else if (RefBio >= ALPHA[sp]) {caught = maxcaught * (RefBio - ALPHA[sp])/(1.0 - ALPHA[sp]);}
               else                             {caught = 0.0;}
               FishingLoss[sp] += caught;
               FishingThru[0]  += caught;
               FishingGain[0]  += caught;
               TerminalF[sp] = caught/state_BB[sp];
            }          
        }
        
   // DETRITUS  - note: check interdetrital flow carefully, have had some issues
 	 // (check by ensuring equlibrium run stays in equilibrium)
	 	  int liv, det;
 		  double flow;
      for (links=1; links<=NumDetLinks; links++){
          liv  = DetFrom[links];
          det  = DetTo[links];
          flow = DetFrac[links] * (MzeroLoss[liv] + UnAssimLoss[liv]);
          DetritalGain[det] += flow;
          if (liv > NUM_LIVING) {DetritalLoss[liv] += flow; }
      }
      for (sp=NUM_LIVING+1; sp<=NUM_LIVING+NUM_DEAD; sp++){
         MzeroLoss[sp] = 0.0;
      }  
    
  // Add mortality forcing
     for (i=1; i<=NUM_DEAD+NUM_LIVING; i++){
        FoodLoss[i]  *= force_bymort(y * STEPS_PER_YEAR + m, i);
        MzeroLoss[i] *= force_bymort(y * STEPS_PER_YEAR + m, i);
     }
  
	// Sum up derivitive parts; move previous derivative to dyt        
     for (i=1; i<=NUM_DEAD+NUM_LIVING; i++){
         dyt[i]=DerivT[i];
         TotGain[i] = FoodGain[i] + DetritalGain[i] + FishingGain[i];      
         LossPropToQ[i] = UnAssimLoss[i]  + ActiveRespLoss[i];
         LossPropToB[i] = FoodLoss[i]     + MzeroLoss[i] +
                                FishingLoss[i]  + DetritalLoss[i]; 
                  
         TotLoss[i] = LossPropToQ[i] + LossPropToB[i];    
         DerivT[i]  = TotGain[i] - TotLoss[i]; 
         
 	   // Set biomeq for "fast equilibrium" of fast variables
  	    if (state_BB[i] > 0) {
            biomeq[i] = TotGain[i] / 
 					                 (TotLoss[i] / state_BB[i]);
        }          
     }     
return 0;
}


//-----#################################################################----
// SplitSetPred function called in sim stanza initialize and update
// This function simply sums up across juvenile and adult age structure to get 
// population-level Biomass, Numbers, and Consumption 
// [[Rcpp::export]]
int SplitSetPred(List mod){

int ageMo, i;  
double Bt, pt, Nt;

  // Parse out List mod
  int juv_N                      = as<int>(mod["juv_N"]);
  NumericVector state_BB         = as<NumericVector>(mod["state_BB"]);
  NumericMatrix WageS            = as<NumericMatrix>(mod["WageS"]);
  NumericVector WWa              = as<NumericVector>(mod["WWa"]);
  NumericMatrix NageS            = as<NumericMatrix>(mod["NageS"]);
  NumericVector state_NN         = as<NumericVector>(mod["state_NN"]);
  NumericVector stanzaPred       = as<NumericVector>(mod["stanzaPred"]);
  NumericVector stanzaGGJuv      = as<NumericVector>(mod["stanzaGGJuv"]);
  NumericVector stanzaGGAdu      = as<NumericVector>(mod["stanzaGGAdu"]);
  NumericVector JuvNum           = as<NumericVector>(mod["JuvNum"]);
  NumericVector AduNum           = as<NumericVector>(mod["AduNum"]);
  NumericVector firstMoJuv       = as<NumericVector>(mod["firstMoJuv"]);
  NumericVector lastMoJuv        = as<NumericVector>(mod["lastMoJuv"]);
  NumericVector firstMoAdu       = as<NumericVector>(mod["firstMoAdu"]);
  NumericVector lastMoAdu        = as<NumericVector>(mod["lastMoAdu"]);
  NumericVector FoodGain         = as<NumericVector>(mod["FoodGain"]);
  
  // loop over split pools
     for (i = 1; i<= juv_N; i++){
         Bt = 1e-30;
         pt = 1e-30;
         Nt = 1e-30;
      // loop over juv monthly age classes
         for (ageMo = firstMoJuv[i]; ageMo <= lastMoJuv[i]; ageMo++){
             Bt = Bt + NageS(ageMo, i) * WageS(ageMo, i);
             pt = pt + NageS(ageMo, i) * WWa(ageMo, i);
             Nt = Nt + NageS(ageMo, i);
             
         }
         state_BB[JuvNum[i]]   = Bt;
         stanzaPred[JuvNum[i]] = pt;
         state_NN[JuvNum[i]]   = Nt;
    
      // loop over adult monthly age classes
         Bt = 1e-30;
         pt = 1e-30;
         Nt = 1e-30;       
         for (ageMo = firstMoAdu[i]; ageMo <= lastMoAdu[i]; ageMo++){
             Bt = Bt + NageS(ageMo, i) * WageS(ageMo, i);
             pt = pt + NageS(ageMo, i) * WWa(ageMo, i);
             Nt = Nt + NageS(ageMo, i);
         }
         state_BB[AduNum[i]]   = Bt;
         stanzaPred[AduNum[i]] = pt;
         state_NN[AduNum[i]]   = Nt;    
         stanzaGGJuv[i] =  FoodGain[JuvNum[i]] / stanzaPred[JuvNum[i]]; 
         stanzaGGAdu[i] =  FoodGain[AduNum[i]] / stanzaPred[AduNum[i]]; 
     }  

return(0);
}


//-----#################################################################----
// Update juvenile adult or "stanza" age structure during sim run 
// on monthly timesteps (not hardwiring months, but recommended)
// [[Rcpp::export]] 
 int update_stanzas(List mod, int yr, int mon){
 
int ageMo, i;  
double Su, Gf, Nt; 
//double propRepro;

// Parse out List mod
  int juv_N                      = as<int>(mod["juv_N"]);
  NumericVector state_BB         = as<NumericVector>(mod["state_BB"]);
  NumericMatrix WageS            = as<NumericMatrix>(mod["WageS"]);
  NumericVector WWa              = as<NumericVector>(mod["WWa"]);
  NumericMatrix NageS            = as<NumericMatrix>(mod["NageS"]);
  NumericVector state_NN         = as<NumericVector>(mod["state_NN"]);
  NumericVector stanzaPred       = as<NumericVector>(mod["stanzaPred"]);
  NumericVector stanzaGGJuv      = as<NumericVector>(mod["stanzaGGJuv"]);
  NumericVector stanzaGGAdu      = as<NumericVector>(mod["stanzaGGAdu"]);
  NumericVector JuvNum           = as<NumericVector>(mod["JuvNum"]);
  NumericVector AduNum           = as<NumericVector>(mod["AduNum"]);
  NumericVector firstMoJuv       = as<NumericVector>(mod["firstMoJuv"]);
  NumericVector lastMoJuv        = as<NumericVector>(mod["lastMoJuv"]);
  NumericVector firstMoAdu       = as<NumericVector>(mod["firstMoAdu"]);
  NumericVector lastMoAdu        = as<NumericVector>(mod["lastMoAdu"]);
  NumericVector FoodGain         = as<NumericVector>(mod["FoodGain"]);
  NumericVector SpawnBio         = as<NumericVector>(mod["SpawnBio"]);
  NumericVector LossPropToB      = as<NumericVector>(mod["LossPropToB"]);
  NumericVector vBM              = as<NumericVector>(mod["vBM"]);
  NumericMatrix SplitAlpha       = as<NumericMatrix>(mod["SplitAlpha"]);
  NumericVector Wmat001          = as<NumericVector>(mod["Wmat001"]);
  NumericVector Wmat50           = as<NumericVector>(mod["Wmat50"]);
  NumericVector Amat001          = as<NumericVector>(mod["Amat001"]);
  NumericVector Amat50           = as<NumericVector>(mod["Amat50"]);
  NumericVector WmatSpread       = as<NumericVector>(mod["WmatSpread"]);
  NumericVector AmatSpread       = as<NumericVector>(mod["AmatSpread"]);
  NumericVector EggsStanza       = as<NumericVector>(mod["EggsStanza"]);
  NumericVector SpawnEnergy      = as<NumericVector>(mod["SpawnEnergy"]);
  NumericVector SpawnX           = as<NumericVector>(mod["SpawnX"]);
  NumericVector baseSpawnBio     = as<NumericVector>(mod["baseSpawnBio"]);
  NumericVector RecMonth         = as<NumericVector>(mod["RecMonth"]);
  NumericVector baseEggsStanza   = as<NumericVector>(mod["baseEggsStanza"]);
  NumericVector RscaleSplit      = as<NumericVector>(mod["RscaleSplit"]);
  NumericVector RzeroS           = as<NumericVector>(mod["RzeroS"]);
  NumericVector RecPower         = as<NumericVector>(mod["RecPower"]);
  NumericVector VonBD            = as<NumericVector>(mod["VonBD"]);
  NumericMatrix force_byrecs     = as<NumericMatrix>(mod["force_byrecs"]);
   
  // loop over split species groups to update n, wt, biomass in sim  
     for (i = 1; i <= juv_N; i++){
         SpawnBio[i] = 0;
      // loop over juv timesteps with juv survival and growth, also calculate Spawning Biomass
         Su = exp(-LossPropToB[JuvNum[i]] / STEPS_PER_YEAR / state_BB[JuvNum[i]]);    
         Gf = FoodGain[JuvNum[i]] / stanzaPred[JuvNum[i]];  // BB should be stanzaPred            
   			 stanzaGGJuv[i] = Gf; 						         
         for (ageMo = firstMoJuv[i]; ageMo <= lastMoJuv[i]; ageMo++){   
             NageS(ageMo, i) = NageS(ageMo, i) * Su;
             WageS(ageMo, i) = vBM[i] * WageS(ageMo, i) + Gf * SplitAlpha(ageMo, i);
             if ( (WageS(ageMo, i) > Wmat001[i]) && (ageMo > Amat001[i])) {
                 SpawnBio[i]  += WageS(ageMo, i) * NageS(ageMo, i) /
                                 (1. + exp(- ((WageS(ageMo, i) - Wmat50[i]) / WmatSpread[i]) 
 																		      - ((ageMo - Amat50[i]) /AmatSpread[i])));                             
             }            
         }
 				      
      // loop over adult timesteps with adult survival and growth, also calculate Spawning Biomass
         Su = exp(-LossPropToB[AduNum[i]] / STEPS_PER_YEAR / state_BB[AduNum[i]]);
         Gf = FoodGain[AduNum[i]] / stanzaPred[AduNum[i]];   //BB should be stanzaPred         
         stanzaGGAdu[i] = Gf; 
         for (ageMo = firstMoAdu[i]; ageMo <= lastMoAdu[i]; ageMo++){
             NageS(ageMo, i) = NageS(ageMo, i) * Su;
             WageS(ageMo, i) = vBM[i] * WageS(ageMo, i) + Gf * SplitAlpha(ageMo, i);
 						if ( (WageS(ageMo, i) > Wmat001[i]) && (ageMo > Amat001[i])) {
                 SpawnBio[i]  += WageS(ageMo, i) * NageS(ageMo, i) /
                                 (1. + exp(- ((WageS(ageMo, i) - Wmat50[i]) / WmatSpread[i]) 
 																		      - ((ageMo - Amat50[i]) / AmatSpread[i])));                             
             }     	       
         }
         
      // KYA This is a Beverton-Holt curve between Spawning Biomass and
      // Number of Eggs Produced; generally near-linear with default parameters        
         EggsStanza[i] = SpawnBio[i] * SpawnEnergy[i] * SpawnX[i] /
 				                             (SpawnX[i] - 1.0 + 
 			  															 (SpawnBio[i] / baseSpawnBio[i]));
 		 // Forcing for eggs applied here															 
          EggsStanza[i] *= force_byrecs(yr * STEPS_PER_YEAR + mon, JuvNum[i]);		

		 // If recruitment happens all at once (in a single month) that happens here)
     // Otherwise, recruitment is throughout the year.
 			  if (RecMonth[i] > 0){
 				   if (mon == RecMonth[i]) {EggsStanza[i] *=1. ;}
 				 else                         {EggsStanza[i] *=0. ;}
 				}
     
     // now update n and wt looping backward over age
         Nt = NageS(lastMoAdu[i], i) + NageS(lastMoAdu[i] - 1, i);
         if (Nt == 0){Nt = 1e-30;}
         WageS(lastMoAdu[i], i) = (WageS(lastMoAdu[i], i) * NageS(lastMoAdu[i], i) + 
                            WageS(lastMoAdu[i] - 1, i) * NageS(lastMoAdu[i] - 1, i)) / Nt;
         NageS(lastMoAdu[i], i) = Nt;
         
         for (ageMo = lastMoAdu[i] - 1; ageMo > firstMoJuv[i]; ageMo--) {
             NageS(ageMo, i) = NageS(ageMo - 1, i);
             WageS(ageMo, i) = WageS(ageMo - 1, i);   
         }
 
      // finally apply number of eggs to youngest juvenile slot. including Carl's recruit power        
         if (baseEggsStanza[i] > 0){
             NageS(firstMoJuv[i], i) = RscaleSplit[i] * RzeroS[i] * 
                                       pow(double(EggsStanza[i] / baseEggsStanza[i]), 
                                       double(RecPower[i]));
         }
         WageS(firstMoJuv[i], i) = 0;
 
     // Uses generalized vonB (exponent is d).    
     // ADDED FOR STABILITY 4/13/07 (Unlucky Friday)
        for (ageMo = 0; ageMo <= lastMoAdu[i]; ageMo++){
             WWa(ageMo, i) = pow(double(WageS(ageMo, i)), double(VonBD[i])); 
         }           
     }  // end of Juvenile Loop  

return(0); 
}


//-----#################################################################----
// [[Rcpp::export]] 
int Adams_Basforth (List mod, int StartYear, int EndYear){
     
int y, m, dd;//c, j, 
int sp, i; //t, ageMo, s, link, prey,pred,links,gr
//int inbound;
double old_B, new_B, pd; //nn, ww, bb, 
//double newbio; float ystep;
//double bioanom, bioratio;
//unsigned int LL;

// Parse out List mod
  int NUM_LIVING = as<int>(mod["NUM_LIVING"]);
  int NUM_DEAD   = as<int>(mod["NUM_DEAD"]);
  int juv_N      = as<int>(mod["juv_N"]);
  int init_run   = as<int>(mod["init_run"]);
  int BURN_YEARS = as<int>(mod["BURN_YEARS"]);
  int CRASH_YEAR = as<int>(mod["CRASH_YEAR"]);
  
  NumericVector B_BaseRef        = as<NumericVector>(mod["B_BaseRef"]);
  NumericVector NoIntegrate      = as<NumericVector>(mod["NoIntegrate"]);
  NumericVector state_BB         = as<NumericVector>(mod["state_BB"]);
  NumericVector state_Ftime      = as<NumericVector>(mod["state_Ftime"]);
  NumericVector FtimeAdj         = as<NumericVector>(mod["FtimeAdj"]);
  NumericVector FtimeQBOpt       = as<NumericVector>(mod["FtimeQBOpt"]);
  NumericMatrix WageS            = as<NumericMatrix>(mod["WageS"]);
  NumericMatrix NageS            = as<NumericMatrix>(mod["NageS"]);
  NumericVector stanzaPred       = as<NumericVector>(mod["stanzaPred"]);
  NumericVector SpawnBio         = as<NumericVector>(mod["SpawnBio"]);
  NumericVector JuvNum           = as<NumericVector>(mod["JuvNum"]);
  NumericVector AduNum           = as<NumericVector>(mod["AduNum"]);
  NumericVector firstMoAdu       = as<NumericVector>(mod["firstMoAdu"]);
  NumericVector DerivT           = as<NumericVector>(mod["DerivT"]);
  NumericVector dyt              = as<NumericVector>(mod["dyt"]);
  NumericVector biomeq           = as<NumericVector>(mod["biomeq"]);
  NumericVector FoodGain         = as<NumericVector>(mod["FoodGain"]);
  NumericVector FishingLoss      = as<NumericVector>(mod["FishingLoss"]);

  NumericMatrix out_BB           = as<NumericMatrix>(mod["out_BB"]);
  NumericMatrix out_CC           = as<NumericMatrix>(mod["out_CC"]);
  NumericMatrix out_SSB          = as<NumericMatrix>(mod["out_SSB"]);
  NumericMatrix out_rec          = as<NumericMatrix>(mod["out_rec"]);
  
   // Update sums of split groups to total biomass for derivative calcs
      SplitSetPred(mod); 
      
    // If not starting from previous timestep, call derivative twice to
 	 // set deriv and deriv(t-1)
      if (init_run){
         deriv_master(mod, 0, 0, 0);
         deriv_master(mod, 0, 0, 0);
      }
      
    // MAIN LOOP STARTS HERE     
       for (y = StartYear; y < EndYear; y++){                                  
         for (m = 0; m < STEPS_PER_YEAR; m++){
                     
           // dd is index for saving monthly output
				      dd = y * STEPS_PER_YEAR + m;
           
           // save current state to output matrix
           // For non-split pools, SSB is output output as the same as B.
					 // For adult split pools, it is overwritten with "actual" SSB, 
					 // for juvs it is 0. 
              for (sp = 1; sp <= NUM_LIVING + NUM_DEAD; sp++){ 
                  out_BB(dd, sp)  = state_BB[sp];
                  out_SSB(dd, sp) = state_BB[sp];
                  out_rec(dd, sp) = state_BB[sp];
              }
              for (i = 1; i <= juv_N; i++){
                  out_SSB(dd, JuvNum[i]) = 0.0;
									out_SSB(dd, AduNum[i]) = SpawnBio[i];
									out_rec(dd, AduNum[i]) = NageS(firstMoAdu[i], i) * WageS(firstMoAdu[i], i);
              }
       
			    // note: code for sub-monthly steps has been removed    
 		      // for (d=0; d<STEPS_PER_MONTH; d++){

 		          // Calculate Derivative for a given timestep
 	               deriv_master(mod, y, m, 0);

 	            // Loop through species, applying Adams-Basforth or fast
 	            // equilibrium depending on species.
 	            
 								 for (sp=1; sp <= NUM_LIVING + NUM_DEAD; sp++){                    

                  // Adjust feeding time 
 	                   if (NoIntegrate[sp] < 0){pd = stanzaPred[sp];}
 	                   else                         {pd = state_BB[sp];}	                      					       
                     if ((pd > 0) && (FoodGain[sp] > 0)){
 										      state_Ftime[sp] = 0.1 + 0.9 * state_Ftime[sp] * 
                                 ((1.0 - FtimeAdj[sp] / (double)STEPS_PER_MONTH) 
 																+ FtimeAdj[sp] / (double)STEPS_PER_MONTH * 
 										             FtimeQBOpt[sp] / (FoodGain[sp] / pd));
                          }	
                     MAX_THRESHOLD(state_Ftime[sp], 2.0);
                      
                  // Biomass update for non-split groups
 								     old_B = state_BB[sp];
 								     new_B = state_BB[sp];
 								     if (NoIntegrate[sp] == 0){
 										 // 'Fast equilibrium'
												new_B        = (1.0 - SORWT) * biomeq[sp] 
 												               + SORWT * state_BB[sp];
 										 }
 										 else if (NoIntegrate[sp] == sp){
                         // Adams-Basforth timestep 
                            new_B = state_BB[sp] + (DELTA_T / 2.0) * 
                                   (3.0 * (DerivT[sp]) - dyt[sp]); 
 										       }
 										       
						      // If the new biomass goes to infinity or something, set a
									// flag to exit the loop and return the problem. 
						         if (std::isnan(new_B) || std::isinf(new_B)) {
                        CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;
                     }
                  
									// If the run is during the "burn-in" years, and biomass goes
									// into the discard range, set flag to exit the loop.  
                     if (y < BURN_YEARS){
                       if  ( (new_B < B_BaseRef[sp] * LO_DISCARD) ||
                             (new_B > B_BaseRef[sp] * HI_DISCARD) ){
                           CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;                         
                       }                          
                     }
                    
                 // Otherwise if biomass goes very large or very small, set to 
                 // min or max.
								    MIN_THRESHOLD(new_B, B_BaseRef[sp] * EPSILON);
                    MAX_THRESHOLD(new_B, B_BaseRef[sp] * BIGNUM);                    
                    
 										state_BB[sp] = new_B;                                
 						        
                 // sum up Catch at every time step (catch is cumulative)
                     if (fabs(state_BB[sp]/old_B-1.0) > EPSILON){
 								       out_CC(dd, sp) = ((FishingLoss[sp] * DELTA_T) / old_B) *
 									                       (state_BB[sp] - old_B) /
 										 	    							 log(state_BB[sp] / old_B);
 									  }
 									  else {out_CC(dd, sp) = (FishingLoss[sp] * DELTA_T);
 								    }		  
  									         
 								 } // End of species loop
		    		       
 					//}  // End of days loop
           
          // Monthly Stanza (split pool) update
 					   update_stanzas(mod, y, m + 1);
             SplitSetPred(mod);
           
				 // As above for non-split groups, check for out-of-bounds, numerical
				 // errors, or burn-in model stoppages for juvs and then adults.   
            for (i = 1; i <= juv_N; i++){
                sp = JuvNum[i];
                new_B = state_BB[sp];
						    if (std::isnan(new_B) || std::isinf(new_B)) {
                   CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;
                }                  
                if (y < BURN_YEARS){
                    if  ( (new_B < B_BaseRef[sp] * LO_DISCARD) ||
                          (new_B > B_BaseRef[sp] * HI_DISCARD) ){
                         CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;                           
                    }                          
                }                
                sp    = AduNum[i];
                new_B = state_BB[sp];
						    if (std::isnan(new_B) || std::isinf(new_B)) {
                    CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;
                }     
                if (y < BURN_YEARS){
                   if  ( (new_B < B_BaseRef[sp] * LO_DISCARD) ||
                         (new_B > B_BaseRef[sp] * HI_DISCARD) ){
                       CRASH_YEAR = y; y = EndYear; m = STEPS_PER_YEAR;                           
                   }                          
                }                
            }
                           
 		    }  // End of main months loop
     
   }// End of years loop
       
   // If you get to the end and haven't crashed, save biomass in the
   // final time slot before exiting.
      if (CRASH_YEAR < 0){
         dd = EndYear * STEPS_PER_YEAR;
         for (sp = 1; sp <= NUM_LIVING + NUM_DEAD; sp++){ 
              out_BB(dd, sp)  = state_BB[sp];
              out_SSB(dd, sp) = state_BB[sp];
              out_rec(dd, sp) = state_BB[sp];
         }
         for (i = 1; i <= juv_N; i++){
              out_SSB(dd, JuvNum[i]) = 0.0;
						  out_SSB(dd, AduNum[i]) = SpawnBio[i];
						  out_rec(dd, AduNum[i]) = NageS(firstMoAdu[i], i) * WageS(firstMoAdu[i], i);
         }
      }  
      
return(0);
} 