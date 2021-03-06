package senpai;

import pfg.config.Config;
import pfg.kraken.exceptions.PathfindingException;
import pfg.kraken.exceptions.StartPointException;
import pfg.kraken.exceptions.TimeoutException;
import pfg.kraken.obstacles.RectangularObstacle;
import pfg.kraken.utils.XY;
import pfg.kraken.utils.XYO;
import pfg.kraken.utils.XY_RW;
import pfg.log.Log;
import senpai.Senpai.ErrorCode;
import senpai.buffer.OutgoingOrderBuffer;
import senpai.comm.CommProtocol;
import senpai.comm.DataTicket;
import senpai.comm.Ticket;
import senpai.exceptions.ActionneurException;
import senpai.exceptions.ScriptException;
import senpai.exceptions.UnableToMoveException;
import senpai.robot.Robot;
import senpai.robot.RobotColor;
import senpai.scripts.Script;
import senpai.scripts.ScriptManager;
import senpai.table.Table;
import senpai.table.TypeAtome;
import senpai.threads.comm.ThreadCommProcess;
import senpai.utils.ConfigInfoSenpai;
import senpai.utils.Severity;
import senpai.utils.Subject;

/*
 * Copyright (C) 2013-2018 Pierre-François Gimenez
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

/**
 * DemoFourche !
 * @author pf
 *
 */

public class DemoFourche
{
	private static Senpai senpai;
	private static OutgoingOrderBuffer ll;
	private static Robot robot;
	private static ScriptManager scripts;
	private static Log log;
	private static Config config;
	private static Table table;

	/**
	 * Gestion des paramètres et de la fermeture du HL, ne pas toucher
	 * @param args
	 */
	public static void main(String[] args)
	{
		ErrorCode error = ErrorCode.NO_ERROR;
		try {
			if(args.length == 1)
				new DemoFourche().exec(args[0]);
			else
			{
				throw new Exception("Paramètre obligatoire : nom du fichier de configuration");
			}
		}
		catch(Exception e)
		{
			e.printStackTrace();
			error = ErrorCode.EXCEPTION;
			error.setException(e);
		}
		finally
		{
			try
			{
				if(senpai != null)
					senpai.destructor(error);
			}
			catch(InterruptedException e)
			{
				e.printStackTrace();
			}
		}
	}
	
	public void exec(String configFile) throws InterruptedException
	{
		/**
		 * Initialisation
		 */
		
		String configfile = configFile;
		
		senpai = new Senpai();
		senpai.initialize(configfile, "default", "graphic");
		config = senpai.getService(Config.class);
		ll = senpai.getService(OutgoingOrderBuffer.class);
		robot = senpai.getService(Robot.class);
		table = senpai.getService(Table.class);
		scripts = senpai.getService(ScriptManager.class);
		log = senpai.getService(Log.class);
		
		RobotColor couleur;			
		
		/**
		 * Initialisation des actionneurs
		 */

		try {
			robot.initActionneurs();
		} catch (ActionneurException e1) {
			log.write("Erreur lors de l'initialisation du bras : "+e1, Subject.STATUS);
		}
		
		/*
		 * Attente de la couleur
		 */

		if(config.getBoolean(ConfigInfoSenpai.DISABLE_JUMPER))
		{
			couleur = RobotColor.VIOLET;
			robot.setDateDebutMatch();
		}
		else
		{
			DataTicket etat;
			do
			{
				// Demande la couleur toute les 100ms et s'arrête dès qu'elle est connue
				Ticket tc = ll.demandeCouleur();
				etat = tc.attendStatus();
				Thread.sleep(100);
			} while(etat.status != CommProtocol.State.OK);
			couleur = (RobotColor) etat.data;
		}
		
		/**
		 * Ajout des zones interdites d'accès
		 */
		XY_RW posZoneDepartAdverse = new XY_RW(-1500+550/2, 1250);
		XY_RW rampeAdverse = new XY_RW(-750, 200);
		if(couleur.symmetry)
		{
			posZoneDepartAdverse.setX(- posZoneDepartAdverse.getX());
			rampeAdverse.setX(- rampeAdverse.getX());
		}
		table.addOtherObstacle(new RectangularObstacle(posZoneDepartAdverse.clone(), 550, 900));
		table.addOtherObstacle(new RectangularObstacle(rampeAdverse.clone(), 1500, 400));

		/**
		 * Ajout des zones où on sait qu'il n'y a aucun ennemi (les mêmes, symétrisés)
		 */
		posZoneDepartAdverse.setX(- posZoneDepartAdverse.getX());
		rampeAdverse.setX(- rampeAdverse.getX());
		table.addNoEnemyZone(new RectangularObstacle(posZoneDepartAdverse.clone(), 550, 900));
		table.addNoEnemyZone(new RectangularObstacle(rampeAdverse.clone(), 1500, 400));

		
		log.write("Couleur utilisée : "+couleur, Subject.STATUS);
		robot.updateColorAndSendPosition(couleur);
		scripts.setCouleur(couleur);
		ll.enableParkingBreak(config.getBoolean(ConfigInfoSenpai.ENABLE_PARKING_BREAK));
		if(config.getBoolean(ConfigInfoSenpai.ENABLE_LIDAR))
			robot.requestLidarCorrection();
		/*
		 * Allumage des capteurs
		 */
		senpai.getService(ThreadCommProcess.class).capteursOn = true;

		/**
		 * Initialisation des scripts
		 */
		Script recuperePaletGreenium = scripts.getScriptRecuperePalet(new XY_RW(1275,285), TypeAtome.Greenium);
		
		/**
		 * script
		 */
		boolean retry = true;
		while (retry)
		{
			try
			{
				retry = false;
				doScript(recuperePaletGreenium, 4, 2, true);
			}
			catch(ScriptException e)
			{
				log.write("Erreur : "+e, Subject.SCRIPT);
				retry = true;
			}
			catch(PathfindingException | UnableToMoveException e)
			{
				log.write("Erreur : "+e, Subject.SCRIPT);
			}
		}
	}
	
	/**
	 * Exécute un script
	 * @param s le script à faire
	 * @param nbEssaiChemin le nombre d'essai de trajet pour y arriver
	 * @param nbEssaiScript le nombre d'essai du script
	 * @param checkFin doit-on vérifier que le robot est arrivé avec une précision suffisante ?
	 * @throws PathfindingException pas de chemin
	 * @throws InterruptedException arrêt de l'utilisateur
	 * @throws UnableToMoveException problème méca lors du trajet
	 * @throws ScriptException problème lors de l'exécution du script
	 */
	private void doScript(Script s, int nbEssaiChemin, int nbEssaiScript, boolean checkFin) throws PathfindingException, InterruptedException, UnableToMoveException, ScriptException
	{
		try {
			// Méthode qui s'occupe de retenter le script
			boolean restartScript;
			do {
				try {
					if(Thread.currentThread().isInterrupted())
						throw new InterruptedException();
	
					restartScript = false;
					doScript(s, nbEssaiChemin, checkFin);
				}
				catch(PathfindingException | UnableToMoveException | ScriptException e)
				{
					nbEssaiScript--;
					if(nbEssaiScript > 0)
						log.write("Erreur lors de l'exécution du script: "+e.getMessage()+", on retente !", Severity.WARNING, Subject.SCRIPT);
					else
					{
						log.write("Erreur lors de l'exécution du script: "+e.getMessage()+", on abandonne !", Severity.WARNING, Subject.SCRIPT);
						throw e;
					}
					restartScript = true;
				}
			} while(restartScript && nbEssaiScript > 0);
		} finally // dans tous les cas, quand on a terminé les tentatives, on replie le bras (si c'est possible)
		{
			try
			{
				robot.rangeSiPossible();
			}
			catch(ActionneurException e)
			{
				log.write("Erreur lors du repliage final lors du script " + s + " : " + e, Severity.CRITICAL, Subject.SCRIPT);
				e.printStackTrace();
			}
		}
	}
		
	/**
	 * Méthode qui s'occupe de retenter Kraken
	 * @param s
	 * @param nbEssaiChemin
	 * @param checkFin
	 * @throws PathfindingException
	 * @throws InterruptedException
	 * @throws UnableToMoveException
	 * @throws ScriptException
	 */
	private void doScript(Script s, int nbEssaiChemin, boolean checkFin) throws PathfindingException, InterruptedException, UnableToMoveException, ScriptException
	{	
		if(Thread.currentThread().isInterrupted())
			throw new InterruptedException();

		log.write("Essai du script "+s, Subject.SCRIPT);
//		if(!s.faisable())
//			throw new ScriptException("Script n'est pas faisable !");
		
		XYO pointEntree = s.getPointEntree();
		log.write("Point d'entrée du script "+pointEntree, Subject.SCRIPT);
		
		double toleranceAngle = s.getToleranceAngle(); // en degré
		double tolerancePosition = s.getTolerancePosition(); // en mm
		double toleranceX = s.getToleranceX();
		double toleranceY = s.getToleranceY();
		
		// si on est déjà bien positionné, on ne fait rien !
		boolean restartKraken = tooFar(robot.getCinematique().getXYO(), pointEntree, tolerancePosition, toleranceAngle, toleranceX, toleranceY);

		while(restartKraken)
		{
			try {
				restartKraken = false;
				robot.goTo(pointEntree, s.reverseSearch());
				XYO corrected = s.correctOdo();
				if(corrected == null)
					corrected = robot.getCinematique().getXYO();

				if(checkFin && !config.getBoolean(ConfigInfoSenpai.SIMULE_COMM))
				{
					// on retente
					if(tooFar(corrected, pointEntree, tolerancePosition, toleranceAngle, toleranceX, toleranceY))
					{
						restartKraken = true;
						nbEssaiChemin--;
						if(nbEssaiChemin > 0)
							log.write("Erreur trop grande, on retente !", Severity.WARNING, Subject.SCRIPT);
						else
						{
							log.write("Erreur trop grande, on abandonne !", Severity.WARNING, Subject.SCRIPT);
							throw new ScriptException("Erreur à l'arrivée trop grande !");
						}
					}
				}

				if(Thread.currentThread().isInterrupted())
					throw new InterruptedException();
			}
			catch(TimeoutException | StartPointException e)
			{
				log.write("Timeout de Kraken : "+e, Severity.WARNING, Subject.SCRIPT);
				helpKraken();
				restartKraken = true;
				nbEssaiChemin--;
				if(nbEssaiChemin == 0)
					throw e;
			}
			catch(PathfindingException e)
			{
				log.write("Exception de Kraken : "+e, Severity.WARNING, Subject.SCRIPT);
				restartKraken = true;
				nbEssaiChemin--;
				if(nbEssaiChemin == 0)
					throw e;
			}
			catch(UnableToMoveException e)
			{
				log.write("Exception durant le trajet : "+e, Severity.WARNING, Subject.SCRIPT);
				restartKraken = true;
				nbEssaiChemin--;
				if(nbEssaiChemin == 0)
					throw e;
			}
		}
		
		s.execute();

		if(Thread.currentThread().isInterrupted())
			throw new InterruptedException();
	}
	
	private boolean tooFar(XYO corrected, XYO pointEntree, double tolerancePosition, double toleranceAngle, double toleranceX, double toleranceY)
	{
		log.write("Erreur en angle: "+Math.abs(XYO.angleDifference(corrected.orientation, pointEntree.orientation))*180/Math.PI+", erreur en position: "+corrected.position.distance(pointEntree.position)+", erreur en X: "+Math.abs(corrected.position.getX() - pointEntree.position.getX())+", erreur en Y: "+Math.abs(corrected.position.getY() - pointEntree.position.getY()), Subject.SCRIPT);
		log.write("Erreur autorisée : "+toleranceAngle+" (angle), "+tolerancePosition+" (position), "+toleranceX+" (X), "+toleranceY+" (Y)", Subject.SCRIPT);
		return (Math.abs(XYO.angleDifference(corrected.orientation, pointEntree.orientation)) > toleranceAngle*Math.PI/180
				|| corrected.position.distance(pointEntree.position) > tolerancePosition
				|| Math.abs(corrected.position.getX() - pointEntree.position.getX()) > toleranceX
				|| Math.abs(corrected.position.getY() - pointEntree.position.getY()) > toleranceY);
	}

	private void helpKraken() throws InterruptedException
	{
		log.write("Aucun script possible, Kraken semble bloqué. On bouge un peu.", Subject.SCRIPT);
		boolean sensAvant = true;
		try
		{
			if(robot.getCinematique().getPosition().minusNewVector(new XY(0,1000)).dot(new XY(100, robot.getCinematique().orientationReelle, false)) < 0)
			{
				sensAvant = true;
				robot.avance(20);
			}
			else
			{
				sensAvant = false;
				robot.avance(-20);
			}
		}
		catch(UnableToMoveException e)
		{
			log.write("Oups… mauvais côté ^^'.", Subject.SCRIPT);
			try {
				if(sensAvant)
					robot.avance(-40);
				else
					robot.avance(40);
			}
			catch(UnableToMoveException e1)
			{
				log.write("Encore bloqué… on attend un peu.", Subject.SCRIPT);
				Thread.sleep(1000);
			}
		}
	}
}
